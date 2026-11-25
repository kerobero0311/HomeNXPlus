// ClaudeNX : Claude en application autonome sur Switch (fonctionne sans HomeNX+).
// Interface texte (console libnx), clavier systeme pour ecrire, reponses en streaming.
//   A : poser une question    Y : nouvelle conversation    X : changer de modele
//   B (pendant une reponse) : arreter    + : quitter
// Fichiers : config/homenxplus/{config.ini, api_key.txt, cacert.pem}
//            homenxplus/chats/  (conversations)   homenxplus/claude/usage-AAAA-MM.csv (couts)
#include "libclaude/client.hpp"

#include <switch.h>
#include <curl/curl.h>

#include <cstdio>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <sstream>

using namespace claude;
namespace fs = std::filesystem;

namespace {

constexpr const char* kCfgDir   = "sdmc:/config/homenxplus";
constexpr const char* kCfgFile  = "sdmc:/config/homenxplus/config.ini";
constexpr const char* kKeyFile  = "sdmc:/config/homenxplus/api_key.txt";
constexpr const char* kCaFile   = "sdmc:/config/homenxplus/cacert.pem";
constexpr const char* kChatsDir = "sdmc:/homenxplus/chats";
constexpr const char* kCostDir  = "sdmc:/homenxplus/claude";
constexpr const char* kLogFile  = "sdmc:/homenxplus/logs/claudenx.log";
constexpr int kMaxHistory = 20;  // messages gardes dans le contexte (limite le cout)

const char* kSystemBase =
    "Tu es Claude, dans l'application ClaudeNX sur une Nintendo Switch. Reponds en francais, "
    "de facon claire et assez courte : l'ecran est petit (80 colonnes, texte brut). "
    "N'utilise pas de tableaux ni de Markdown complexe ; des listes simples avec des tirets suffisent.";

std::string readFile(const char* path) {
    std::ifstream in(path, std::ios::binary);
    std::stringstream ss; ss << in.rdbuf();
    return ss.str();
}

void logLine(const std::string& line) {  // jamais la cle ni le contenu des messages
    std::error_code ec; fs::create_directories("sdmc:/homenxplus/logs", ec);
    std::ofstream(kLogFile, std::ios::app) << std::time(nullptr) << " " << line << "\n";
}

std::string nowIso() {
    std::time_t t = std::time(nullptr); char b[32];
    std::strftime(b, sizeof(b), "%Y-%m-%dT%H:%M:%S", std::localtime(&t));
    return b;
}
std::string monthTag() {
    std::time_t t = std::time(nullptr); char b[16];
    std::strftime(b, sizeof(b), "%Y-%m", std::localtime(&t));
    return b;
}
std::string ledgerPath() { return std::string(kCostDir) + "/usage-" + monthTag() + ".csv"; }

void writeDefaultConfig() {
    std::error_code ec; fs::create_directories(kCfgDir, ec);
    if (fs::exists(kCfgFile, ec)) return;
    std::ofstream(kCfgFile) <<
        "; Configuration de ClaudeNX / HomeNX+ (Claude)\n"
        "; Modele par defaut (rapide et economique) et modele alternatif (touche X)\n"
        "model = claude-haiku-5-5\n"
        "model_alt = claude-opus-5-5\n"
        "; Longueur maximale d'une reponse (jetons)\n"
        "max_tokens = 4096\n"
        "; Effort de reflexion : low, medium ou high\n"
        "effort = low\n"
        "; Plafond de depense mensuel en euros (Claude se bloque au-dela)\n"
        "monthly_cap_eur = 5\n"
        "usd_to_eur = 0.92\n";
}

// Affichage avec retour a la ligne par mot (console de 80 colonnes).
struct Writer {
    int col = 0;
    std::string word;
    void flushWord() {
        if (word.empty()) return;
        if (col + (int)word.size() > 79 && col > 0) { std::printf("\n"); col = 0; }
        std::printf("%s", word.c_str()); col += (int)word.size(); word.clear();
    }
    void put(const std::string& utf8) {
        for (char c : asciiFold(utf8)) {
            if (c == '\n') { flushWord(); std::printf("\n"); col = 0; }
            else if (c == ' ') { flushWord(); if (col < 79) { std::printf(" "); ++col; } }
            else word.push_back(c);
        }
    }
    void end() { flushWord(); std::printf("\n"); col = 0; }
};

// Derniere capture d'ecran de l'Album (carte SD puis memoire interne). Lecture seule.
bool loadLastScreenshot(std::vector<std::uint8_t>& jpeg, std::string& when) {
    if (R_FAILED(capsaInitialize())) return false;
    bool found = false; CapsAlbumEntry best{};
    auto key = [](const CapsAlbumFileDateTime& d) {
        return ((((u64)d.year * 100 + d.month) * 100 + d.day) * 100 + d.hour) * 10000ULL + d.minute * 100ULL + d.second;
    };
    for (CapsAlbumStorage st : {CapsAlbumStorage_Sd, CapsAlbumStorage_Nand}) {
        u64 count = 0;
        if (R_FAILED(capsaGetAlbumFileCount(st, &count)) || count == 0) continue;
        std::vector<CapsAlbumEntry> list(count); u64 got = 0;
        if (R_FAILED(capsaGetAlbumFileList(st, &got, list.data(), count))) continue;
        for (u64 i = 0; i < got; ++i) {
            const auto& e = list[i];
            if (e.file_id.content != CapsAlbumFileContents_ScreenShot) continue;
            if (!found || key(e.file_id.datetime) > key(best.file_id.datetime)) { best = e; found = true; }
        }
    }
    if (found) {
        u64 size = 0, out = 0;
        if (R_SUCCEEDED(capsaGetAlbumFileSize(&best.file_id, &size)) && size > 0 && size < 4 * 1024 * 1024) {
            jpeg.resize(size);
            found = R_SUCCEEDED(capsaLoadAlbumFile(&best.file_id, &out, jpeg.data(), size)) && out > 0;
            jpeg.resize(found ? out : 0);
            const auto& d = best.file_id.datetime; char b[40];
            std::snprintf(b, sizeof(b), "%02u/%02u/%04u %02u:%02u", d.day, d.month, d.year, d.hour, d.minute);
            when = b;
        } else found = false;
    }
    capsaExit();
    return found;
}

struct App {
    PadState pad{};
    std::vector<std::uint8_t> attached;  // capture jointe a la prochaine question (ZR), puis oubliee
    std::string attachedWhen;
    Config cfg;
    std::string key;
    bool useAlt = false;
    std::vector<Message> history;
    std::string chatFile;
    double monthUsd = 0;

    const std::string& model() const { return useAlt ? cfg.modelAlt : cfg.model; }

    void header() {
        consoleClear();
        std::printf("\x1b[1;36mClaudeNX\x1b[0m  -  HomeNX+  -  modele : %s\n", model().c_str());
        std::printf("Depense du mois : %.2f EUR / plafond %.2f EUR\n", monthUsd * cfg.usdToEur, cfg.monthlyCapEur);
        std::printf("A : question   ZR : joindre la derniere capture   Y : nouvelle conversation\n");
        std::printf("X : changer de modele   B : arreter une reponse   + : quitter\n");
        std::printf("--------------------------------------------------------------------------------\n");
    }

    void newChat() {
        history.clear();
        std::time_t t = std::time(nullptr); char b[40];
        std::strftime(b, sizeof(b), "claudenx-%Y%m%d-%H%M%S.json", std::localtime(&t));
        chatFile = std::string(kChatsDir) + "/" + b;
    }

    void saveChat() {
        std::error_code ec; fs::create_directories(kChatsDir, ec);
        json j; j["app"] = "ClaudeNX"; j["model"] = model(); j["messages"] = json::array();
        for (auto& m : history) {
            json c = m.content;  // l'image (lourde) n'est pas recopiee dans l'historique sur la carte
            if (c.is_array()) for (auto& b : c) if (b.value("type", "") == "image") b = {{"type", "text"}, {"text", "[capture d'ecran jointe]"}};
            j["messages"].push_back({{"role", m.role}, {"content", c}});
        }
        std::ofstream(chatFile) << j.dump(1);
    }

    bool askText(std::string& out) {
        SwkbdConfig kbd; char buf[2048] = {0};
        if (R_FAILED(swkbdCreate(&kbd, 0))) return false;
        swkbdConfigMakePresetDefault(&kbd);
        swkbdConfigSetHeaderText(&kbd, "Question pour Claude");
        swkbdConfigSetGuideText(&kbd, "Ecris ta question");
        swkbdConfigSetStringLenMax(&kbd, 2000);
        Result rc = swkbdShow(&kbd, buf, sizeof(buf));
        swkbdClose(&kbd);
        if (R_FAILED(rc) || !buf[0]) return false;
        out = buf;
        return true;
    }

    void ask() {
        if (key.empty()) {
            std::printf("\n\x1b[1;33mAucune cle API.\x1b[0m Cree-la sur console.anthropic.com (avec une limite de\n"
                        "depenses), puis mets-la seule dans le fichier :\n  sd:/config/homenxplus/api_key.txt\n");
            return;
        }
        if (capReached(monthUsd, cfg.monthlyCapEur, cfg.usdToEur)) {
            std::printf("\n\x1b[1;31mPlafond mensuel atteint\x1b[0m (%.2f EUR). Modifie monthly_cap_eur dans config.ini.\n", cfg.monthlyCapEur);
            return;
        }
        std::string q;
        if (!askText(q)) return;

        if (!attached.empty()) {
            // Capture choisie par l'utilisateur (ZR) : envoyee une seule fois, avec la question
            history.push_back({"user", json::array({imageBlockJpeg(attached), {{"type", "text"}, {"text", q}}})});
        } else {
            history.push_back({"user", q});
        }
        while ((int)history.size() > kMaxHistory) history.erase(history.begin(), history.begin() + 2);

        Writer w;
        std::printf("\n\x1b[1;32mToi :\x1b[0m ");
        if (!attached.empty()) { std::printf("[capture du %s] ", attachedWhen.c_str()); attached.clear(); }
        w.put(q); w.end();
        std::printf("\x1b[1;35mClaude :\x1b[0m "); consoleUpdate(nullptr);

        RequestOptions opt; opt.model = model(); opt.maxTokens = cfg.maxTokens; opt.effort = cfg.effort;
        opt.system = kSystemBase; if (!cfg.system.empty()) opt.system += "\n" + cfg.system;

        StreamState st;
        auto onEvent = [&](const SseEvent& ev) {
            std::string delta = st.apply(ev);
            if (!delta.empty()) { w.put(delta); consoleUpdate(nullptr); }
            padUpdate(&pad);
            return !(padGetButtonsDown(&pad) & HidNpadButton_B);  // B : arreter
        };
        HttpResult r = postStream(key, buildRequestBody(opt, history), kCaFile, onEvent);
        w.end();

        if (r.netError || r.status >= 400 || st.failed()) {
            std::string type = st.errorType();
            if (type.empty() && !r.errorBody.empty()) {
                json e = json::parse(r.errorBody, nullptr, false);
                if (!e.is_discarded()) type = e.value("error", json::object()).value("type", "");
            }
            std::printf("\x1b[1;31m%s\x1b[0m\n", errorMessageFr(r.status, type, r.netError).c_str());
            logLine("erreur http=" + std::to_string(r.status) + " net=" + std::to_string(r.netError) + " type=" + type);
            history.pop_back();  // la question sera reposee
            return;
        }
        if (!st.content().empty()) history.push_back({"assistant", st.content()});  // blocs renvoyes tels quels
        else history.pop_back();
        std::string fin = stopReasonFr(st.stopReason());
        if (!fin.empty()) std::printf("\x1b[1;33m%s\x1b[0m\n", fin.c_str());

        const double usd = costUsd(model(), st.usage());
        monthUsd += usd;
        std::error_code ec; fs::create_directories(kCostDir, ec);
        std::ofstream(ledgerPath(), std::ios::app) << ledgerLine(nowIso(), model(), st.usage(), usd);
        std::printf("\x1b[2m[%lld jetons entree, %lld sortie : %.4f EUR | mois : %.2f EUR]\x1b[0m\n",
                    (long long)st.usage().input, (long long)st.usage().output, usd * cfg.usdToEur, monthUsd * cfg.usdToEur);
        saveChat();
        logLine("reponse ok modele=" + model() + " sortie=" + std::to_string(st.usage().output));
    }
};

} // namespace

int main(int, char**) {
    consoleInit(nullptr);
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    socketInitializeDefault();
    curl_global_init(CURL_GLOBAL_DEFAULT);

    App app;
    padInitializeDefault(&app.pad);
    writeDefaultConfig();
    app.cfg = parseConfig(readFile(kCfgFile));
    app.key = sanitizeApiKey(readFile(kKeyFile));
    app.monthUsd = ledgerTotalUsd(readFile(ledgerPath().c_str()));
    app.newChat();
    app.header();

    if (app.key.empty())
        std::printf("Pas encore de cle API : appuie sur A pour savoir comment l'ajouter.\n");
    std::error_code ec;
    if (!fs::exists(kCaFile, ec))
        std::printf("\x1b[1;33mAttention : %s manque (certificats HTTPS).\x1b[0m\n", kCaFile);
    std::printf("\nAppuie sur A pour poser ta premiere question.\n");

    while (appletMainLoop()) {
        padUpdate(&app.pad);
        u64 down = padGetButtonsDown(&app.pad);
        if (down & HidNpadButton_Plus) break;
        if (down & HidNpadButton_A) app.ask();
        if (down & HidNpadButton_Y) { app.newChat(); app.header(); std::printf("Nouvelle conversation.\n"); }
        if (down & HidNpadButton_X) { app.useAlt = !app.useAlt; app.header(); std::printf("Modele : %s\n", app.model().c_str()); }
        if (down & HidNpadButton_ZR) {
            if (!app.attached.empty()) { app.attached.clear(); std::printf("\nCapture retiree.\n"); }
            else if (loadLastScreenshot(app.attached, app.attachedWhen))
                std::printf("\nCapture du %s jointe (%u Ko) : elle partira avec ta prochaine question (A).\n"
                            "ZR a nouveau pour l'annuler.\n", app.attachedWhen.c_str(), (unsigned)(app.attached.size() / 1024));
            else std::printf("\nAucune capture d'ecran trouvee dans l'Album.\n");
        }
        consoleUpdate(nullptr);
    }

    curl_global_cleanup();
    socketExit();
    consoleExit(nullptr);
    return 0;
}
