// HomeNX+ : onglet de reglages « HomeNX+ » (Claude, scenes, batterie, rangement, a propos).
#include "TabBuilders.hpp"

#include "hnp/claude_tasks.hpp"
#include "hnp/layout.hpp"
#include "hnp/notes.hpp"
#include "hnp/switch_games.hpp"
#include "hnp/pclink.hpp"
#include "hnp/safemode.hpp"
#include "hnp/scenes.hpp"
#include "hnp/switch_actions.hpp"
#include "libclaude/claude.hpp"

#include <switch.h>

#include <ctime>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>

namespace {

constexpr const char* kLayout  = "sdmc:/homenxplus/layout.json";
constexpr const char* kScenes  = "sdmc:/homenxplus/scenes.json";
constexpr const char* kCards   = "sdmc:/config/CardsNX/settings.json";
constexpr const char* kKey     = "sdmc:/config/homenxplus/api_key.txt";
constexpr const char* kCfg     = "sdmc:/config/homenxplus/config.ini";
constexpr const char* kClaude  = "sdmc:/switch/ClaudeNX/ClaudeNX.nro";

std::string readAll(const std::string& p) {
    std::ifstream in(p, std::ios::binary); std::stringstream ss; ss << in.rdbuf(); return ss.str();
}

hnp::Scenes loadScenes() {
    nlohmann::json j = nlohmann::json::parse(readAll(kScenes), nullptr, false);
    return hnp::Scenes::fromJson(j.is_discarded() ? nlohmann::json() : j);
}

void saveScenes(const hnp::Scenes& s) {
    std::error_code ec; std::filesystem::create_directories("sdmc:/homenxplus", ec);
    hnp::saveWithBackups(kScenes, s.toJson().dump(1), 2);
}

std::string monthSpend(double& capEur) {
    claude::Config cfg = claude::parseConfig(readAll(kCfg));
    capEur = cfg.monthlyCapEur;
    std::time_t t = std::time(nullptr); char m[16]; std::strftime(m, sizeof(m), "%Y-%m", std::localtime(&t));
    double usd = claude::ledgerTotalUsd(readAll(std::string("sdmc:/homenxplus/claude/usage-") + m + ".csv"));
    char b[64]; std::snprintf(b, sizeof(b), "%.2f EUR / plafond %.2f EUR", usd * cfg.usdToEur, cfg.monthlyCapEur);
    return b;
}

std::string layoutSummary() {
    std::error_code ec;
    if (!std::filesystem::exists(kLayout, ec)) return "Pas encore importé";
    nlohmann::json j = nlohmann::json::parse(readAll(kLayout), nullptr, false);
    hnp::Layout l = hnp::Layout::fromJson(j.is_discarded() ? nlohmann::json() : j);
    return std::to_string(l.home.size()) + " éléments, " + std::to_string(l.folders.size()) + " dossiers, " +
           std::to_string(l.hidden.size()) + " masqués";
}

} // namespace

SettingsScreen::Tab settings::tabs::HomeNXPlusTab::build(SettingsScreen& screen) {
    using SettingItem = SettingsScreen::SettingItem;
    using ItemType = SettingsScreen::ItemType;
    SettingsScreen::Tab t;
    t.name = "HomeNX+";
    auto section = [&](const char* name) { SettingItem it; it.label = name; it.type = ItemType::Section; t.items.push_back(std::move(it)); };
    auto info = [&](std::string label, std::string value, std::string desc = "") {
        SettingItem it; it.label = std::move(label); it.type = ItemType::Info; it.infoText = std::move(value);
        it.description = std::move(desc); t.items.push_back(std::move(it));
    };

    // ------------------------------------------------------------ Style d'affichage
    section("Style d'affichage");
    {
        SettingItem it;
        it.label = "Apparence : Wii U (HomeNX)";
        it.buttonLabel = "Passer aux Cartes";
        it.description = "L'apparence Cartes (CardsNX) avec le panneau HomeNX+ (trois points en haut à gauche). "
                         "Le menu redémarre en 2 secondes ; reviens ici depuis Cartes > trois points > Style.";
        it.type = ItemType::Action;
        it.onChange = [&screen](SettingItem& self) {
            {
                std::ofstream out(HNP_CFG_SWITCHU "/frontend.txt", std::ios::trunc);
                out << "cards";
                if (!out) { self.description = "Impossible d'enregistrer le style sur la carte SD."; return; }
            }
            const std::string why = screen.m_restartMenuCb ? screen.m_restartMenuCb() : std::string("Relance le menu pour voir les Cartes.");
            if (!why.empty()) {
                std::ofstream(HNP_CFG_SWITCHU "/frontend.txt", std::ios::trunc) << "homenx";  // rien n'a change
                self.description = why;
            }
        };
        t.items.push_back(std::move(it));
    }

    // ------------------------------------------------------------ Claude
    section("Claude");
    {
        std::error_code ec;
        const bool key = !claude::sanitizeApiKey(readAll(kKey)).empty();
        info("Clé API", key ? "Présente" : "Absente",
             key ? "" : "Crée-la sur console.anthropic.com (avec une limite de dépenses), puis mets-la seule dans "
                        "sd:/config/homenxplus/api_key.txt.");
        double cap = 5;
        info("Dépense du mois", monthSpend(cap), "Claude se bloque au-delà du plafond (monthly_cap_eur dans config.ini).");
        SettingItem it;
        it.label = "Ouvrir ClaudeNX";
        it.buttonLabel = "Ouvrir";
        it.description = std::filesystem::exists(kClaude, ec)
            ? "Pose tes questions à Claude. ZR joint ta dernière capture d'écran."
            : "ClaudeNX n'est pas installé (switch/ClaudeNX/ClaudeNX.nro).";
        it.type = ItemType::Action;
        it.enabled = std::filesystem::exists(kClaude, ec);
        it.onChange = [&screen](SettingItem&) { if (screen.m_launchNroCb) screen.m_launchNroCb(kClaude); };
        t.items.push_back(std::move(it));
    }

    // ------------------------------------------------------------ Dernier jeu : notes + checklist
    {
        const std::uint64_t tid = hnp::lastPlayedGame();
        char idb[17]; std::snprintf(idb, sizeof idb, "%016lX", (unsigned long)tid);
        const std::string id = idb, name = tid ? hnp::gameName(tid) : "";
        section(tid ? ("Dernier jeu : " + name).c_str() : "Dernier jeu");
        if (!tid) {
            info("Aucun jeu récent", "", "Joue à un jeu puis reviens ici pour écrire tes notes.");
        } else {
            auto nj = nlohmann::json::parse(readAll(hnp::notesPath(id)), nullptr, false);
            const hnp::GameNotes notes = hnp::GameNotes::fromJson(nj.is_discarded() ? nlohmann::json() : nj);
            SettingItem edit;
            edit.label = "Mes notes";
            edit.buttonLabel = "Modifier";
            edit.description = notes.text.empty() ? "Aucune note. Elles s'affichent aussi en jeu dans l'overlay HomeNX+." : notes.text;
            edit.wrapLabel = true;
            edit.type = ItemType::Action;
            edit.onChange = [id](SettingItem& self) {
                auto j = nlohmann::json::parse(readAll(hnp::notesPath(id)), nullptr, false);
                hnp::GameNotes n = hnp::GameNotes::fromJson(j.is_discarded() ? nlohmann::json() : j);
                SwkbdConfig kbd; char buf[2048] = {0};
                if (R_FAILED(swkbdCreate(&kbd, 0))) return;
                swkbdConfigMakePresetDefault(&kbd);
                swkbdConfigSetHeaderText(&kbd, "Notes du jeu");
                swkbdConfigSetInitialText(&kbd, n.text.c_str());
                swkbdConfigSetStringLenMax(&kbd, 2000);
                Result rc = swkbdShow(&kbd, buf, sizeof(buf));
                swkbdClose(&kbd);
                if (R_FAILED(rc)) return;
                n.titleId = id; n.text = buf; n.updated = (std::int64_t)std::time(nullptr);
                std::error_code ec; std::filesystem::create_directories("sdmc:/homenxplus/notes", ec);
                hnp::saveWithBackups(hnp::notesPath(id), n.toJson().dump(1), 1);
                self.description = n.text.empty() ? "Notes effacées." : n.text;
            };
            t.items.push_back(std::move(edit));

            auto cj = nlohmann::json::parse(readAll(hnp::checklistPath(id)), nullptr, false);
            const hnp::Checklist cl = hnp::Checklist::fromJson(cj.is_discarded() ? nlohmann::json() : cj);
            SettingItem gen;
            gen.label = cl.items.empty() ? "Checklist de complétion" : "Checklist : " + std::to_string(cl.percent()) + " % (" +
                        std::to_string(cl.doneCount()) + "/" + std::to_string(cl.items.size()) + ")";
            gen.buttonLabel = cl.items.empty() ? "Créer avec Claude" : "Refaire avec Claude";
            gen.description = "Claude propose la liste des objectifs à 100 % de ce jeu (quelques centimes au plus). "
                              "Tu la coches ensuite en jeu dans l'overlay HomeNX+.";
            gen.type = ItemType::Action;
            gen.onChange = [id, name](SettingItem& self) {
                std::string reply;
                if (!hnp::askClaudeOnce(hnp::checklistPrompt(name), reply)) { self.description = reply; return; }
                hnp::Checklist c = hnp::Checklist::fromText(id, reply);
                if (c.items.empty() || (c.items.size() == 1 && c.items[0].label == "Jeu inconnu")) {
                    self.description = "Claude ne connaît pas assez ce jeu pour une checklist fiable."; return;
                }
                std::error_code ec; std::filesystem::create_directories("sdmc:/homenxplus/checklists", ec);
                hnp::saveWithBackups(hnp::checklistPath(id), c.toJson().dump(1), 1);
                self.label = "Checklist : 0 % (0/" + std::to_string(c.items.size()) + ")";
                self.description = std::to_string(c.items.size()) + " objectifs enregistrés. Ouvre l'overlay HomeNX+ en jeu pour les cocher.";
            };
            t.items.push_back(std::move(gen));

            SettingItem tr;
            tr.label = "Traduire ma dernière capture";
            tr.buttonLabel = "Traduire";
            tr.description = "Prends une capture en jeu (bouton Capture), puis Claude traduit son texte en français. "
                             "La traduction est ajoutée à tes notes de ce jeu (visible dans l'overlay).";
            tr.type = ItemType::Action;
            tr.wrapLabel = true;
            tr.onChange = [id](SettingItem& self) {
                std::vector<std::uint8_t> jpeg; std::string when, reply;
                if (!hnp::lastScreenshot(jpeg, when)) { self.description = "Aucune capture d'écran trouvee dans l'Album."; return; }
                if (!hnp::askClaudeOnce(hnp::translatePrompt(), reply, 1500, &jpeg)) { self.description = reply; return; }
                auto j = nlohmann::json::parse(readAll(hnp::notesPath(id)), nullptr, false);
                hnp::GameNotes n = hnp::GameNotes::fromJson(j.is_discarded() ? nlohmann::json() : j);
                n.titleId = id; n.updated = (std::int64_t)std::time(nullptr);
                n.text += (n.text.empty() ? "" : "\n\n") + std::string("[Traduction capture du ") + when + "]\n" + reply;
                std::error_code ec; std::filesystem::create_directories("sdmc:/homenxplus/notes", ec);
                hnp::saveWithBackups(hnp::notesPath(id), n.toJson().dump(1), 1);
                self.description = reply;
            };
            t.items.push_back(std::move(tr));

            const hnp::PcLink pcl = hnp::parsePcLink(readAll("sdmc:/config/homenxplus/pc_agent.ini"));
            SettingItem sv;
            sv.label = "Copier ses saves sur le PC";
            sv.buttonLabel = "Copier";
            sv.description = "Copie la sauvegarde de ce jeu dans Documents\\Switch_Sauvegardes (lecture seule, pc_agent lancé).";
            sv.type = ItemType::Action;
            sv.enabled = pcl.configured();
            sv.onChange = [pcl, tid, name](SettingItem& self) { self.description = hnp::sendSavesToPc(pcl, tid, name); };
            t.items.push_back(std::move(sv));
        }
    }

    // ------------------------------------------------------------ Dossiers intelligents (apercu)
    section("Dossiers intelligents");
    {
        std::map<std::string, hnp::Item> items;
        for (auto& it : hnp::installedGames()) items[it.id] = it;
        auto names = [&](const std::vector<std::string>& ids, std::size_t n, bool hours) {
            std::string s;
            for (std::size_t i = 0; i < ids.size() && i < n; ++i) {
                const auto& it = items[ids[i]];
                s += "- " + it.title + (hours ? " (" + std::to_string(it.playMinutes / 60) + " h)" : "") + "\n";
            }
            return s.empty() ? std::string("(aucun)") : s;
        };
        const auto recents = hnp::smartFolder(hnp::Smart::Recent, items, {});
        const auto most = hnp::smartFolder(hnp::Smart::MostPlayed, items, {});
        const auto never = hnp::smartFolder(hnp::Smart::NeverLaunched, items, {});
        info("Récents", std::to_string(recents.size()) + " jeux", names(recents, 5, false));
        info("Les plus joués", std::to_string(most.size()) + " jeux", names(most, 5, true));
        info("Jamais lancés", std::to_string(never.size()) + " jeux", names(never, 8, false));
        for (auto& it : t.items) if (it.type == ItemType::Info && (it.label == "Récents" || it.label == "Les plus joués" || it.label == "Jamais lancés")) it.wrapLabel = true;
    }

    // ------------------------------------------------------------ Scenes
    section("Scènes");
    {
        hnp::Scenes scenes = loadScenes();
        SettingItem it;
        it.label = "Scène active";
        it.type = ItemType::Selector;
        it.options.push_back("Aucune (profils sys-clk)");
        it.intVal = 0;
        for (const auto& s : scenes.list()) {
            if (s.id == scenes.active) it.intVal = (int)it.options.size();
            it.options.push_back(s.name);
        }
        it.description = hnp::sysclkRunning()
            ? "Nuit, Batterie, Perf max ou tes scènes : fréquences (sys-clk) et luminosité d'un coup."
            : "sys-clk ne tourne pas : seule la luminosité sera appliquée.";
        it.onChange = [](SettingItem& self) {
            hnp::Scenes sc = loadScenes();
            std::string msg;
            if (self.intVal <= 0 || self.intVal > (int)sc.list().size()) { msg = hnp::clearScene(); sc.active.clear(); }
            else { const auto& s = sc.list()[(std::size_t)self.intVal - 1]; msg = hnp::applyScene(s); sc.active = s.id; }
            saveScenes(sc);
            self.description = msg;
        };
        t.items.push_back(std::move(it));
        info("Mes scènes", std::to_string(scenes.list().size()) + " scènes",
             "Modifiables dans sd:/homenxplus/scenes.json (créées au premier choix d'une scène).");
    }

    // ------------------------------------------------------------ Batterie
    section("Batterie");
    {
        hnp::BatteryStatus b = hnp::readBattery();
        std::string val = b.percent < 0 ? "Inconnue" : std::to_string(b.percent) + " %" + (b.charging ? " (en charge)" : "");
        std::string desc = "Avec l'AutoRCM, une batterie vide oblige à réinjecter Hekate depuis le PC.";
        if (b.percent >= 0 && !b.charging && b.percent <= 15) {
            hnp::BatteryGuard g; desc = hnp::BatteryGuard::message(g.update(b.percent, false));
        }
        info("Niveau", val, desc);
    }

    // ------------------------------------------------------------ Rangement
    section("Rangement");
    {
        info("Rangement HomeNX+", layoutSummary(), "Dossiers, favoris, tris et recherche arrivent dans le menu dans une prochaine version.");
        SettingItem it;
        it.label = "Importer mon rangement CardsNX";
        it.buttonLabel = "Importer";
        it.description = "Recopie l'ordre de tes jeux et les cartes masquées de CardsNX (sans rien modifier dans CardsNX).";
        it.type = ItemType::Action;
        it.onChange = [](SettingItem& self) {
            nlohmann::json cards = nlohmann::json::parse(readAll(kCards), nullptr, false);
            if (cards.is_discarded()) { self.description = "CardsNX : réglages introuvables ou illisibles."; return; }
            hnp::Layout l = hnp::Layout::importCardsNX(cards);
            std::error_code ec; std::filesystem::create_directories("sdmc:/homenxplus", ec);
            bool ok = hnp::saveWithBackups(kLayout, l.toJson().dump(1));
            self.description = ok ? ("Importé : " + std::to_string(l.home.size()) + " jeux, " + std::to_string(l.hidden.size()) +
                                     " masqués (sd:/homenxplus/layout.json).")
                                  : "Échec de l'écriture sur la carte.";
        };
        t.items.push_back(std::move(it));
    }

    // ------------------------------------------------------------ A propos
    // ------------------------------------------------------------ PC (pc_agent + Wake-on-LAN)
    // ------------------------------------------------------------ Médias (comme la page Médias des Cartes)
    section("Médias : Netflix, Disney+, Prime Video, Spotify");
    {
        const hnp::PcLink pc = hnp::parsePcLink(readAll("sdmc:/config/homenxplus/pc_agent.ini"));
        std::error_code ec;
        std::string moon = "sdmc:/switch/Moonlight/Moonlight-Switch.nro";
        if (!std::filesystem::exists(moon, ec)) moon.clear();
        const std::pair<const char*, const char*> kServices[] = {
            {"netflix", "Netflix"}, {"disney", "Disney+"}, {"prime", "Prime Video"}, {"spotify", "Spotify"}};
        for (const auto& [id, name] : kServices) {
            SettingItem it;
            it.label = name;
            it.buttonLabel = "Regarder";
            it.description = pc.configured() ? "S'ouvre sur le PC, puis Moonlight l'affiche sur la Switch."
                                             : "Il faut d'abord relier le PC (section PC ci-dessous).";
            it.type = ItemType::Action;
            it.enabled = pc.configured();
            const std::string sid = id, label = name;
            it.onChange = [&screen, pc, sid, label, moon](SettingItem& self) {
                if (!hnp::pingPc(pc)) {  // pas d'attente ici : le réveil prend 10 à 30 s
                    self.description = pc.mac.empty() ? std::string("Le PC ne répond pas : allume-le et lance pc_agent.")
                                                      : hnp::wakePc(pc) + " Réessaie dans 30 secondes.";
                    return;
                }
                std::string msg;
                if (!hnp::openOnPc(pc, sid, msg)) { self.description = msg; return; }
                if (moon.empty()) { self.description = label + " est ouvert sur le PC. Moonlight n'est pas installé."; return; }
                self.description = label + " est ouvert sur le PC : Moonlight démarre, choisis Bureau (Desktop).";
                if (screen.m_launchNroCb) screen.m_launchNroCb(moon);
            };
            t.items.push_back(std::move(it));
        }
        info("Pourquoi par le PC ?", "",
             "Ces services n'ont pas d'appli Switch et le navigateur de la console ne gère pas leur protection anti-copie. "
             "Si l'image est noire dans Moonlight, désactive l'accélération matérielle du navigateur du PC.");
        for (auto& it : t.items) if (it.label == "Pourquoi par le PC ?") it.wrapLabel = true;
    }

    section("PC");
    {
        const hnp::PcLink pc = hnp::parsePcLink(readAll("sdmc:/config/homenxplus/pc_agent.ini"));
        info("Lien avec le PC", pc.configured() ? (hnp::pingPc(pc) ? "Connecté (" + pc.ip + ")" : "Configuré, PC éteint ou pc_agent fermé")
                                                : "Non configuré",
             pc.configured() ? "" : "Lance pc_agent sur le PC puis outils/installer_sur_sd.ps1 : il remplit config/homenxplus/pc_agent.ini.");
        SettingItem wake;
        wake.label = "Réveiller le PC";
        wake.buttonLabel = "Réveiller";
        wake.description = pc.mac.empty() ? "Adresse MAC du PC inconnue (pc_agent.ini)." : "Envoie un signal Wake-on-LAN au PC (" + pc.mac + ").";
        wake.type = ItemType::Action;
        wake.enabled = !pc.mac.empty();
        wake.onChange = [pc](SettingItem& self) { self.description = hnp::wakePc(pc); };
        t.items.push_back(std::move(wake));

        SettingItem send;
        send.label = "Envoyer un message au PC";
        send.buttonLabel = "Écrire";
        send.description = "Le message arrive dans Documents\\Switch_Sauvegardes\\boite\\de_switch sur le PC.";
        send.type = ItemType::Action;
        send.enabled = pc.configured();
        send.onChange = [pc](SettingItem& self) {
            SwkbdConfig kbd; char buf[1024] = {0};
            if (R_FAILED(swkbdCreate(&kbd, 0))) { self.description = "Clavier indisponible."; return; }
            swkbdConfigMakePresetDefault(&kbd);
            swkbdConfigSetHeaderText(&kbd, "Message pour le PC");
            Result rc = swkbdShow(&kbd, buf, sizeof(buf));
            swkbdClose(&kbd);
            if (R_SUCCEEDED(rc) && buf[0]) self.description = hnp::sendToPc(pc, buf);
        };
        t.items.push_back(std::move(send));

        SettingItem caps;
        caps.label = "Envoyer mes captures au PC";
        caps.buttonLabel = "Envoyer";
        caps.description = "Copie sur le PC les 30 dernières captures d'écran pas encore envoyées (l'Album n'est pas modifié).";
        caps.type = ItemType::Action;
        caps.enabled = pc.configured();
        caps.onChange = [pc](SettingItem& self) { self.description = hnp::sendCapturesToPc(pc, 30); };
        t.items.push_back(std::move(caps));

        SettingItem recv;
        recv.label = "Messages du PC";
        recv.buttonLabel = "Lire";
        recv.description = "Messages déposés par le PC (dossier boite\\vers_switch).";
        recv.type = ItemType::Action;
        recv.enabled = pc.configured();
        recv.onChange = [pc](SettingItem& self) {
            int n = 0;
            std::string all = hnp::readFromPc(pc, n);
            self.description = n ? std::to_string(n) + " message(s) :\n" + all : "Aucun nouveau message (ou PC injoignable).";
            self.wrapLabel = true;
        };
        t.items.push_back(std::move(recv));
    }

    section("Stabilité");
    info("Mode sans échec", hnp::g_safeMode ? "ACTIF" : "Inactif",
         hnp::g_safeMode ? "Le menu a planté 3 fois de suite : verre, musique et images SteamGridDB sont coupés pour ce démarrage."
                         : "S'active tout seul après 3 démarrages ratés de suite.");

    section("À propos");
    info("Version", "HomeNX+ 0.9.0");
    info("Base", "HomeNX (ad3333m) / SwitchU (PoloNX), GPL-3.0");
    return t;
}
