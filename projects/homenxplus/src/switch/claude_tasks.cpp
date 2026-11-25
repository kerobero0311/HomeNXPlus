// HomeNX+ : demandes ponctuelles a Claude depuis le menu.
#include "hnp/claude_tasks.hpp"

#include "libclaude/client.hpp"

#include <switch.h>

#include <algorithm>

#include <ctime>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace hnp {
namespace {
std::string readAll(const std::string& p) { std::ifstream in(p, std::ios::binary); std::stringstream ss; ss << in.rdbuf(); return ss.str(); }
std::string ledgerPath() {
    std::time_t t = std::time(nullptr); char m[16]; std::strftime(m, sizeof m, "%Y-%m", std::localtime(&t));
    return std::string("sdmc:/homenxplus/claude/usage-") + m + ".csv";
}
}

std::string checklistPrompt(const std::string& gameName) {
    return "Donne la checklist de completion a 100 % du jeu \"" + gameName + "\" sur Nintendo Switch. "
           "Reponds uniquement par la liste : une tache courte par ligne, chaque ligne commence par \"- \", "
           "30 taches maximum, en francais, sans introduction ni conclusion. "
           "Si tu ne connais pas ce jeu, reponds seulement \"- Jeu inconnu\".";
}

bool askClaudeOnce(const std::string& prompt, std::string& out, int maxTokens, const std::vector<std::uint8_t>* jpeg) {
    const std::string key = claude::sanitizeApiKey(readAll("sdmc:/config/homenxplus/api_key.txt"));
    if (key.empty()) { out = "Pas de cle API : mets-la dans sd:/config/homenxplus/api_key.txt."; return false; }
    claude::Config cfg = claude::parseConfig(readAll("sdmc:/config/homenxplus/config.ini"));
    const double spent = claude::ledgerTotalUsd(readAll(ledgerPath()));
    if (claude::capReached(spent, cfg.monthlyCapEur, cfg.usdToEur)) { out = "Plafond mensuel atteint (config.ini)."; return false; }

    claude::RequestOptions opt; opt.model = cfg.model; opt.maxTokens = maxTokens; opt.effort = cfg.effort;
    opt.system = "Tu aides l'utilisateur de HomeNX+ (menu de sa Nintendo Switch). Reponses en francais, en texte brut.";
    claude::StreamState st;
    claude::json content = prompt;
    if (jpeg && !jpeg->empty()) content = claude::json::array({claude::imageBlockJpeg(*jpeg), {{"type", "text"}, {"text", prompt}}});
    auto r = claude::postStream(key, claude::buildRequestBody(opt, {{"user", content}}), "sdmc:/config/homenxplus/cacert.pem",
                                [&](const claude::SseEvent& ev) { st.apply(ev); return true; });
    if (r.netError || r.status >= 400 || st.failed()) {
        std::string type = st.errorType();
        if (type.empty() && !r.errorBody.empty()) {
            auto e = claude::json::parse(r.errorBody, nullptr, false);
            if (!e.is_discarded()) type = e.value("error", claude::json::object()).value("type", "");
        }
        out = claude::errorMessageFr(r.status, type, r.netError);
        return false;
    }
    const double usd = claude::costUsd(opt.model, st.usage());
    std::error_code ec; std::filesystem::create_directories("sdmc:/homenxplus/claude", ec);
    std::time_t t = std::time(nullptr); char iso[32]; std::strftime(iso, sizeof iso, "%Y-%m-%dT%H:%M:%S", std::localtime(&t));
    std::ofstream(ledgerPath(), std::ios::app) << claude::ledgerLine(iso, opt.model, st.usage(), usd);
    out = st.text();
    if (out.empty()) { out = claude::stopReasonFr(st.stopReason()); return false; }
    return true;
}

std::string translatePrompt() {
    return "Voici une capture d'ecran d'un jeu Nintendo Switch. Traduis en francais tout le texte visible "
           "(dialogues, menus, indications), dans l'ordre de lecture, en texte brut. "
           "S'il n'y a pas de texte, decris en une phrase ce que montre l'image.";
}

bool lastScreenshot(std::vector<std::uint8_t>& jpeg, std::string& when) {
    if (R_FAILED(capsaInitialize())) return false;
    bool found = false; CapsAlbumEntry best{};
    auto key = [](const CapsAlbumFileDateTime& d) {
        return ((((u64)d.year * 100 + d.month) * 100 + d.day) * 100 + d.hour) * 10000ULL + d.minute * 100ULL + d.second;
    };
    for (CapsAlbumStorage st : {CapsAlbumStorage_Sd, CapsAlbumStorage_Nand}) {
        u64 count = 0;
        if (R_FAILED(capsaGetAlbumFileCount(st, &count)) || count == 0) continue;
        std::vector<CapsAlbumEntry> l(count); u64 got = 0;
        if (R_FAILED(capsaGetAlbumFileList(st, &got, l.data(), count))) continue;
        for (u64 i = 0; i < got; ++i)
            if (l[i].file_id.content == CapsAlbumFileContents_ScreenShot && (!found || key(l[i].file_id.datetime) > key(best.file_id.datetime))) { best = l[i]; found = true; }
    }
    if (found) {
        u64 size = 0, out = 0;
        found = R_SUCCEEDED(capsaGetAlbumFileSize(&best.file_id, &size)) && size > 0 && size < 4 * 1024 * 1024;
        if (found) {
            jpeg.resize(size);
            found = R_SUCCEEDED(capsaLoadAlbumFile(&best.file_id, &out, jpeg.data(), size)) && out > 0;
            jpeg.resize(found ? out : 0);
            const auto& d = best.file_id.datetime; char b[40];
            std::snprintf(b, sizeof b, "%02u/%02u/%04u %02u:%02u", d.day, d.month, d.year, d.hour, d.minute);
            when = b;
        }
    }
    capsaExit();
    return found;
}

} // namespace hnp
