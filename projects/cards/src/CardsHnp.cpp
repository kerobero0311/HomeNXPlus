// HomeNX+ : ses ajouts dans l'apparence Cartes (CardsNX).
//  - trois points > HomeNX+ et Reglages > HomeNX+ : Claude, scenes, PC, rangement, sante ;
//  - trois points > Style : passer a l'apparence Wii U (et inversement depuis celle-ci) ;
//  - + sur un jeu > Fiche HomeNX+ : notes, checklist, Claude, saves.
// Les taches longues (reseau, Claude) tournent sur un fil a part : le menu ne gele jamais.
// Compile seulement dans le menu HomeNX+ (HNP_PLUS, xmake.lua), pas dans CardsNX seul.
#ifdef HNP_PLUS

#include "CardsActivity.hpp"

#include "hnp/assist.hpp"
#include "hnp/claude_tasks.hpp"
#include "hnp/layout.hpp"
#include "hnp/notes.hpp"
#include "hnp/pclink.hpp"
#include "hnp/safemode.hpp"
#include "hnp/scenes.hpp"
#include "hnp/services.hpp"
#include "hnp/switch_actions.hpp"
#include "hnp/switch_games.hpp"
#include "libclaude/claude.hpp"

#include <nlohmann/json.hpp>
#include <nxui/Application.hpp>
#include <switch.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <sstream>

namespace cards {
namespace {

constexpr const char* kScenesFile = "sdmc:/homenxplus/scenes.json";
constexpr const char* kKeyFile    = "sdmc:/config/homenxplus/api_key.txt";
constexpr const char* kCfgFile    = "sdmc:/config/homenxplus/config.ini";
constexpr const char* kPcFile     = "sdmc:/config/homenxplus/pc_agent.ini";
constexpr const char* kClaudeNro  = "sdmc:/switch/ClaudeNX/ClaudeNX.nro";
constexpr const char* kSelfNro    = "sdmc:/switch/HomeNXPlus/HomeNXPlus.nro";  // le .nro de test
constexpr const char* kStyleFile  = HNP_CFG_SWITCHU "/frontend.txt";
constexpr const char* kVersion    = "HomeNX+ 0.9.3";
constexpr const char* kSceneAuto  = "sdmc:/homenxplus/scene_auto.txt";  // scene posee au lancement d'un jeu

std::string readAll(const std::string& p) {
    std::ifstream in(p, std::ios::binary);
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

std::string hex(std::uint64_t id) {
    char b[17];
    std::snprintf(b, sizeof b, "%016lX", static_cast<unsigned long>(id));
    return b;
}

hnp::Scenes loadScenes() {
    nlohmann::json j = nlohmann::json::parse(readAll(kScenesFile), nullptr, false);
    return hnp::Scenes::fromJson(j.is_discarded() ? nlohmann::json() : j);
}

void saveScenes(const hnp::Scenes& s) {
    std::error_code ec;
    std::filesystem::create_directories("sdmc:/homenxplus", ec);
    hnp::saveWithBackups(kScenesFile, s.toJson().dump(1), 2);
}

hnp::GameNotes loadNotes(const std::string& id) {
    auto j = nlohmann::json::parse(readAll(hnp::notesPath(id)), nullptr, false);
    return hnp::GameNotes::fromJson(j.is_discarded() ? nlohmann::json() : j);
}

void saveNotes(hnp::GameNotes n) {
    n.updated = static_cast<std::int64_t>(std::time(nullptr));
    std::error_code ec;
    std::filesystem::create_directories("sdmc:/homenxplus/notes", ec);
    hnp::saveWithBackups(hnp::notesPath(n.titleId), n.toJson().dump(1), 1);
}

hnp::Checklist loadChecklist(const std::string& id) {
    auto j = nlohmann::json::parse(readAll(hnp::checklistPath(id)), nullptr, false);
    return hnp::Checklist::fromJson(j.is_discarded() ? nlohmann::json() : j);
}

void saveChecklist(const hnp::Checklist& c) {
    std::error_code ec;
    std::filesystem::create_directories("sdmc:/homenxplus/checklists", ec);
    hnp::saveWithBackups(hnp::checklistPath(c.titleId), c.toJson().dump(1), 1);
}

bool hasKey() { return !claude::sanitizeApiKey(readAll(kKeyFile)).empty(); }

std::string monthSpend() {
    claude::Config cfg = claude::parseConfig(readAll(kCfgFile));
    std::time_t t = std::time(nullptr);
    char m[16];
    std::strftime(m, sizeof m, "%Y-%m", std::localtime(&t));
    const double usd = claude::ledgerTotalUsd(readAll(std::string("sdmc:/homenxplus/claude/usage-") + m + ".csv"));
    char b[64];
    std::snprintf(b, sizeof b, "%.2f / %.2f EUR", usd * cfg.usdToEur, cfg.monthlyCapEur);
    return b;
}

hnp::PcLink pcLink() { return hnp::parsePcLink(readAll(kPcFile)); }

std::string checklistText(const hnp::Checklist& c) {
    if (c.items.empty()) return "Pas encore de checklist pour ce jeu.";
    std::string s = std::to_string(c.percent()) + " % fait (" + std::to_string(c.doneCount()) + "/" +
                    std::to_string(c.items.size()) + ")\n\n";
    for (const auto& it : c.items) s += std::string(it.done ? "[x] " : "[ ] ") + it.label + "\n";
    s += "\nCoche les objectifs en jeu dans l'overlay HomeNX+ (Ultrahand).";
    return s;
}

const char* kNoKey = "Pas de clé API : crée-la sur console.anthropic.com (avec une limite de dépenses), "
                     "puis mets-la seule dans sd:/config/homenxplus/api_key.txt.";

} // namespace

// ---------------------------------------------------------------- taches de fond

void CardsActivity::hnpRun(const std::string& busy, const std::string& title, std::function<std::string()> job, bool reader,
                           std::function<void(const std::string&)> done) {
    if (m_hnpJob.valid()) {
        say("Une action HomeNX+ est déjà en cours, patiente un instant.");
        return;
    }
    m_hnpJobTitle = title;
    m_hnpJobReader = reader;
    m_hnpDone = std::move(done);
    m_hnpBusy = busy;
    m_hnpJob = std::async(std::launch::async, [job = std::move(job)]() {
        try {
            return job();
        } catch (const std::exception& e) {
            return std::string("Erreur : ") + e.what();
        }
    });
    say(busy);
}

void CardsActivity::hnpPoll() {
    const std::uint64_t now = armGetSystemTick();
    if (!m_hnpStarted) { // une scene appliquee au lancement d'un jeu s'arrete au retour au menu
        m_hnpStarted = true;
        std::error_code ec;
#ifdef SWITCHU_MENU
        const bool gameOpen = runningId() != 0;
#else
        const bool gameOpen = false;
#endif
        if (!gameOpen && std::filesystem::exists(kSceneAuto, ec)) {
            std::filesystem::remove(kSceneAuto, ec);
            hnpRun("Retour aux réglages normaux...", "", []() {
                hnp::Scenes x = loadScenes();
                x.active.clear();
                saveScenes(x);
                return hnp::clearScene();
            }, false);
        }
    }
    // Garde batterie (AutoRCM) : une alerte a 15 % puis a 8 %, toutes les 30 s au plus
    if (now - m_hnpBatTick > armNsToTicks(30'000'000'000ULL)) {
        m_hnpBatTick = now;
        static hnp::BatteryGuard guard;
        const hnp::BatteryStatus b = hnp::readBattery();
        if (b.percent >= 0) {
            const hnp::BatteryAlert alert = guard.update(b.percent, b.charging);
            if (alert != hnp::BatteryAlert::None) {
                notifyCard("Batterie faible", hnp::BatteryGuard::message(alert), "power");
                say(hnp::BatteryGuard::message(alert));
            }
        }
    }
    if (!m_hnpJob.valid()) return;
    if (m_hnpJob.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
        if (m_messageFor < 1.f && !m_hnpBusy.empty()) say(m_hnpBusy); // toujours visible tant que ca tourne
        return;
    }
    m_hnpBusy.clear();
    const std::string out = m_hnpJob.get();
    m_messageFor = 0.f; // (le message Â« en cours Â» laisse la place)
    if (m_hnpDone) {
        auto done = std::move(m_hnpDone);
        m_hnpDone = nullptr;
        done(out);
    } else if (m_hnpJobReader) openReader(m_hnpJobTitle, out, "trophy");
    else say(out);
    app().requestRedraw();
}

// ---------------------------------------------------------------- style

void CardsActivity::hnpSwitchStyle(bool toCards) {
#ifdef SWITCHU_MENU
    if (runningId()) { // le daemon ne relance le menu que si rien n'est ouvert
        say("Ferme d'abord le jeu ou l'appli ouverte (+ > Fermer) : le menu doit redémarrer.");
        return;
    }
#endif
    {
        std::error_code ec;
        std::filesystem::create_directories(std::filesystem::path(kStyleFile).parent_path(), ec);
        std::ofstream out(kStyleFile, std::ios::trunc);
        out << (toCards ? "cards" : "homenx");
        if (!out) {
            say("Impossible d'enregistrer le style sur la carte SD.");
            return;
        }
    }
    m_settings.save();
    flushWrites();
    m_sheetOpen = false;
    m_dotsOpen = false;
    say(toCards ? "Apparence Cartes : le menu redémarre..." : "Apparence Wii U : le menu redémarre...");
    leave(-1, 0.5f, [this]() {
#ifndef SWITCHU_MENU
        // Le .nro de test se relance lui-meme (le menu HOME, lui, est relance par le daemon).
        std::error_code ec;
        if (envHasNextLoad() && std::filesystem::exists(kSelfNro, ec))
            envSetNextLoad(kSelfNro, (std::string("\"") + kSelfNro + "\"").c_str());
#endif
        app().requestExit();
    });
}

void CardsActivity::openHnpStyle() {
    std::vector<MenuItem> items;
    MenuItem now = info("Apparence actuelle", [] { return std::string("Cartes (CardsNX)"); });
    items.push_back(std::move(now));
    MenuItem wiiu = action("Passer à l'apparence Wii U (HomeNX)", [this]() { hnpSwitchStyle(false); });
    wiiu.detail = "Grille de jeux, dossiers, réglages complets. Le menu redémarre (2 s).";
    wiiu.glyph = "copy";
    items.push_back(std::move(wiiu));
    MenuItem look = action("Couleurs, verre, fond, animations…", [this]() {
        openSettings();
        for (size_t i = 0; i < m_sheet.size(); ++i)
            if (m_sheet[i].name == "Look") m_sheetSec = static_cast<int>(i);
    });
    look.detail = "Les réglages d'apparence des Cartes";
    look.glyph = "sun";
    items.push_back(std::move(look));
    MenuItem pages = action("Pages et ordre des jeux", [this]() { openSettings(0); });
    pages.glyph = "sort";
    items.push_back(std::move(pages));
    items.push_back(action("Retour", [] {}));
    openMenu("Style d'affichage", std::move(items));
}

// ---------------------------------------------------------------- Claude

void CardsActivity::openHnpClaude() {
    std::vector<MenuItem> items;
    const bool key = hasKey();
    items.push_back(info("Clé API", [key] { return std::string(key ? "Présente" : "Absente"); }));
    items.push_back(info("Dépense du mois", [] { return monthSpend(); }));
    if (!key) {
        items.push_back(action("Comment l'installer", [this]() { openReader("Clé API Claude", kNoKey, "news"); }));
    } else {
        MenuItem ask = action("Poser une question", [this]() {
            askText("Question pour Claude", "Écris ta question", "", 500, [this](const std::string& q) {
                if (q.find_first_not_of(" \r\n\t") == std::string::npos) return;
                hnpRun("Claude réfléchit...", "Claude", [q]() {
                    std::string out;
                    hnp::askClaudeOnce(q, out, 1500);
                    return "Toi : " + q + "\n\n" + out;
                }, true);
            });
        });
        ask.detail = "La réponse s'affiche ici (quelques centimes au plus)";
        ask.glyph = "news";
        items.push_back(std::move(ask));
        MenuItem help = action("Que faire ici ? (dernière capture)", [this]() {
            openMenu("Envoyer ta dernière capture à Claude ?", {
                info("Seule la dernière capture de l'Album part", [] { return std::string(); }),
                action("Oui, envoyer", [this]() {
                    hnpRun("Claude regarde ta capture...", "Que faire ici ?", []() {
                        std::vector<std::uint8_t> jpeg;
                        std::string when, out;
                        if (!hnp::lastScreenshot(jpeg, when)) return std::string("Aucune capture d'écran dans l'Album : appuie sur le bouton Capture en jeu.");
                        hnp::askClaudeOnce("Voici une capture d'écran de mon jeu Nintendo Switch. Explique simplement "
                                           "ce que je dois faire ici pour avancer, en 5 etapes au plus, en français.",
                                           out, 1200, &jpeg);
                        return "Capture du " + when + "\n\n" + out;
                    }, true);
                }),
                action("Annuler", [] {}),
            });
        });
        help.glyph = "sys_album";
        items.push_back(std::move(help));
        MenuItem trad = action("Traduire ma dernière capture", [this]() {
            openMenu("Envoyer ta dernière capture à Claude ?", {
                info("Claude traduit son texte en français", [] { return std::string(); }),
                action("Oui, traduire", [this]() {
                    hnpRun("Traduction en cours...", "Traduction", []() {
                        std::vector<std::uint8_t> jpeg;
                        std::string when, out;
                        if (!hnp::lastScreenshot(jpeg, when)) return std::string("Aucune capture d'écran dans l'Album.");
                        if (!hnp::askClaudeOnce(hnp::translatePrompt(), out, 1500, &jpeg)) return out;
                        if (const std::uint64_t tid = hnp::lastPlayedGame()) { // gardee dans les notes du dernier jeu
                            hnp::GameNotes n = loadNotes(hex(tid));
                            n.titleId = hex(tid);
                            n.text += (n.text.empty() ? "" : "\n\n") + std::string("[Traduction capture du ") + when + "]\n" + out;
                            saveNotes(n);
                        }
                        return "Capture du " + when + "\n\n" + out;
                    }, true);
                }),
                action("Annuler", [] {}),
            });
        });
        trad.glyph = "tag";
        items.push_back(std::move(trad));
    }
    std::error_code ec;
    if (std::filesystem::exists(kClaudeNro, ec)) {
        MenuItem nx = action("Ouvrir ClaudeNX (conversations)", [this]() { openNro(kClaudeNro); });
        nx.glyph = "play";
        items.push_back(std::move(nx));
    }
    items.push_back(action("Retour", [this]() { openHnp(); }));
    openMenu("Claude", std::move(items));
}

// ---------------------------------------------------------------- fiche de jeu

void CardsActivity::openHnpGame(std::uint64_t id, const std::string& name) {
    const std::string tid = hex(id);
    std::vector<MenuItem> items;
    const hnp::Checklist cl = loadChecklist(tid);
    MenuItem read = action("Lire mes notes", [this, tid, name]() {
        const hnp::GameNotes n = loadNotes(tid);
        openReader("Notes : " + name, n.text.empty() ? "Aucune note pour ce jeu." : n.text, "tag");
    });
    read.glyph = "news";
    items.push_back(std::move(read));
    MenuItem add = action("Ajouter une note", [this, tid]() {
        askText("Note du jeu", "Elle s'affiche aussi en jeu (overlay HomeNX+)", "", 500, [this, tid](const std::string& t) {
            if (t.find_first_not_of(" \r\n\t") == std::string::npos) return;
            hnp::GameNotes n = loadNotes(tid);
            n.titleId = tid;
            n.text += (n.text.empty() ? "" : "\n") + t;
            saveNotes(n);
            m_audio.play("toggle_on");
            say("Note enregistrée.");
        });
    });
    add.glyph = "tag";
    items.push_back(std::move(add));
    MenuItem clear = action("Effacer mes notes", [this, tid]() {
        openMenu("Effacer les notes de ce jeu ?", {
            action("Effacer", [this, tid]() {
                hnp::GameNotes n = loadNotes(tid);
                n.titleId = tid;
                n.text.clear();
                saveNotes(n); // (les 1 versions precedentes restent : notes.json.1)
                say("Notes effacées.");
            }, true),
            action("Annuler", [] {}),
        });
    }, true);
    clear.glyph = "trash";
    items.push_back(std::move(clear));
    if (!cl.items.empty()) {
        MenuItem tick = action("Cocher la checklist", [this, tid, name]() { openHnpChecklist(tid, name); });
        tick.glyph = "trophy";
        items.push_back(std::move(tick));
    }
    MenuItem see = action(cl.items.empty() ? "Checklist de complétion" :
                          "Checklist : " + std::to_string(cl.percent()) + " %", [this, tid, name]() {
        openReader("Checklist : " + name, checklistText(loadChecklist(tid)), "trophy");
    });
    see.glyph = "trophy";
    items.push_back(std::move(see));
    if (hasKey()) {
        MenuItem gen = action(cl.items.empty() ? "Créer la checklist avec Claude" : "Refaire la checklist avec Claude",
                              [this, tid, name]() {
            hnpRun("Claude prépare la checklist...", "Checklist : " + name, [tid, name]() {
                std::string reply;
                if (!hnp::askClaudeOnce(hnp::checklistPrompt(name), reply)) return reply;
                hnp::Checklist c = hnp::Checklist::fromText(tid, reply);
                if (c.items.empty() || (c.items.size() == 1 && c.items[0].label == "Jeu inconnu"))
                    return std::string("Claude ne connaît pas assez ce jeu pour une checklist fiable.");
                c.source = "claude";
                saveChecklist(c);
                return std::to_string(c.items.size()) + " objectifs enregistrés.\n\n" + checklistText(c);
            }, true);
        });
        gen.detail = "Quelques centimes au plus";
        gen.glyph = "news";
        items.push_back(std::move(gen));
        MenuItem ask = action("Demander à Claude sur ce jeu", [this, name]() {
            askText("Question sur " + name, "Ex. : où trouver la clé du donjon ?", "", 500, [this, name](const std::string& q) {
                if (q.find_first_not_of(" \r\n\t") == std::string::npos) return;
                hnpRun("Claude réfléchit...", name, [q, name]() {
                    std::string out;
                    hnp::askClaudeOnce("Je joue à \"" + name + "\" sur Nintendo Switch. " + q +
                                       " Réponds simplement, sans gâcher la suite si ce n'est pas demandé.", out, 1500);
                    return "Toi : " + q + "\n\n" + out;
                }, true);
            });
        });
        ask.glyph = "friends";
        items.push_back(std::move(ask));
    }
    const hnp::PcLink pc = pcLink();
    if (pc.configured()) {
        MenuItem saves = action("Copier ses saves sur le PC", [this, pc, id, name]() {
            hnpRun("Copie de la sauvegarde vers le PC...", "", [pc, id, name]() { return hnp::sendSavesToPc(pc, id, name); }, false);
        });
        saves.detail = "Lecture seule, vers Documents\\Switch_Sauvegardes (pc_agent lancé)";
        saves.glyph = "download";
        items.push_back(std::move(saves));
    }
    // Scene automatique de ce jeu (appliquee quand il est lance depuis HomeNX+)
    {
        hnp::Scenes sc = loadScenes();
        const auto per = sc.forGame(tid);
        std::string cur = "Aucune";
        if (per)
            if (const hnp::Scene* s = sc.find(*per)) cur = s->name;
        MenuItem scene = choice("Scène de ce jeu", [cur] { return cur; }, [this, tid, id, name](int) {
            hnp::Scenes s = loadScenes();
            std::vector<MenuItem> pick;
            pick.push_back(action("Aucune", [this, tid, id, name]() {
                hnp::Scenes x = loadScenes();
                x.setForGame(tid, "");
                saveScenes(x);
                openHnpGame(id, name);
            }));
            for (const hnp::Scene& one : s.list()) {
                const std::string sid = one.id;
                pick.push_back(action(one.name, [this, tid, sid, id, name]() {
                    hnp::Scenes x = loadScenes();
                    x.setForGame(tid, sid);
                    saveScenes(x);
                    openHnpGame(id, name);
                }));
            }
            openMenu("Scène pour " + name, std::move(pick));
        }, "Fréquences et luminosité pour ce jeu");
        items.push_back(std::move(scene));
    }
    items.push_back(action("Fermer", [] {}));
    openMenu(name, std::move(items));
}

void CardsActivity::openHnpChecklist(const std::string& tid, const std::string& name) {
    auto list = std::make_shared<hnp::Checklist>(loadChecklist(tid));
    std::vector<MenuItem> items;
    for (std::size_t i = 0; i < list->items.size(); ++i) {
        items.push_back(toggle(list->items[i].label, [list, i] { return list->items[i].done; },
                               [this, list, i]() {
                                   list->toggle(i);
                                   saveChecklist(*list);
                                   m_audio.play(list->items[i].done ? "toggle_on" : "toggle_off");
                               }));
    }
    items.push_back(action("Fermer", [] {}));
    openMenu("Checklist : " + name, std::move(items));
}

void CardsActivity::hnpSceneForLaunch(std::uint64_t id) {
    const std::string tid = hex(id);
    const hnp::Scenes sc = loadScenes();
    const auto per = sc.forGame(tid);
    if (!per || per->empty()) return;
    const hnp::Scene* s = sc.find(*per);
    if (!s) return;
    hnp::applyScene(*s);  // quelques appels IPC : immediat
    hnp::Scenes x = sc;
    x.active = s->id;
    saveScenes(x);
    std::error_code ec;
    std::filesystem::create_directories("sdmc:/homenxplus", ec);
    std::ofstream(kSceneAuto) << s->id;
}

// ---------------------------------------------------------------- rangement assisté par Claude

void CardsActivity::hnpAssistSort() {
    std::vector<std::string> titles;
    std::map<std::string, std::string> idOf;  // titre -> id hexa
    for (const Game& g : m_pages[kGames])
        if (g.path.empty() && g.action.empty() && !g.homebrew && idOf.emplace(g.name, idText(g.id)).second) titles.push_back(g.name);
    if (titles.size() < 4) {
        say("Il faut au moins 4 jeux pour les ranger.");
        return;
    }
    hnpRun("Claude range tes jeux...", "", [titles]() {
        std::string out;
        return hnp::askClaudeOnce(hnp::categoriesPrompt(titles), out, 4000) ? out : "ERREUR " + out;
    }, false, [this, titles, idOf](const std::string& reply) {
        if (reply.rfind("ERREUR ", 0) == 0) { say(reply.substr(7)); return; }
        const hnp::Categories cats = hnp::parseCategories(reply, titles, Settings::kMaxCustomPages);
        if (cats.empty()) { say("La proposition de Claude n'est pas lisible : réessaie."); return; }
        std::vector<MenuItem> items;
        for (const auto& [name, games] : cats) {
            std::string list;
            for (size_t i = 0; i < games.size() && i < 4; ++i) list += (i ? ", " : "") + games[i];
            if (games.size() > 4) list += "...";
            MenuItem row = info(name + " : " + std::to_string(games.size()) + " jeux", [] { return std::string(); });
            row.detail = list;
            items.push_back(std::move(row));
        }
        items.push_back(action("Créer ces pages", [this, cats, idOf]() {
            int made = 0, updated = 0, skipped = 0;
            for (const auto& [name, games] : cats) {
                std::vector<std::string> ids;
                for (const std::string& t : games)
                    if (const auto f = idOf.find(t); f != idOf.end()) ids.push_back(f->second);
                auto page = std::find_if(m_settings.customPages.begin(), m_settings.customPages.end(),
                                         [&](const CustomPage& p) { return p.name == name; });
                if (page != m_settings.customPages.end()) { page->titles = std::move(ids); ++updated; }
                else if (m_settings.customPages.size() < Settings::kMaxCustomPages) { m_settings.customPages.push_back({name, std::move(ids)}); ++made; }
                else ++skipped;
            }
            m_settings.normalizePages();
            m_settings.save();
            rebuildCustomPages();
            m_audio.play("toggle_on");
            say(std::to_string(made + updated) + " page(s) prête(s) : ZL / ZR pour y aller."
                + (skipped ? " " + std::to_string(skipped) + " sans place (8 pages perso au plus)." : std::string()));
        }));
        items.back().glyph = "copy";
        items.push_back(action("Non merci", [] {}));
        openMenu("Proposition de Claude", std::move(items));
    });
}

// ---------------------------------------------------------------- jeux du PC (réveil puis Moonlight)

void CardsActivity::hnpPlayPc() {
    std::string moon = "sdmc:/switch/Moonlight/Moonlight-Switch.nro";
    std::error_code ec;
    if (!std::filesystem::exists(moon, ec)) moon = findNro("moonlight");
    if (moon.empty()) { say("Moonlight n'est pas installé sur la Switch."); return; }
    const hnp::PcLink pc = pcLink();
    if (!pc.configured()) { openNro(moon); return; }  // pas de lien : Moonlight directement
    hnpRun("Réveil du PC (jusqu'à 1 min)...", "", [pc]() {
        if (hnp::pingPc(pc)) return std::string("1");
        if (pc.mac.empty()) return std::string("0Le PC ne répond pas et son adresse MAC est inconnue.");
        hnp::wakePc(pc);
        for (int i = 0; i < 20; ++i) {
            svcSleepThread(3'000'000'000LL);
            if (hnp::pingPc(pc)) return std::string("1");
        }
        return std::string("0Le PC ne s'est pas réveillé : Moonlight démarre quand même.");
    }, false, [this, moon](const std::string& r) {
        if (r.size() > 1) say(r.substr(1));
        openNro(moon);
    });
}

// ---------------------------------------------------------------- recherche (Y, clavier USB)
// Idee reprise de tomvita/SwitchU (filtre par nom + clavier USB), ecrite ici pour les Cartes.

void CardsActivity::openHnpSearch(const std::string& initial) {
    m_dotsOpen = false;
    askText("Rechercher un jeu ou une appli", "Une partie du nom suffit, sans accents si tu veux", initial, 64,
            [this](const std::string& q) { hnpSearchResults(q); });
}

void CardsActivity::hnpSearchResults(const std::string& query) {
    auto fold = [](const std::string& s) {
        std::string out = claude::asciiFold(s);
        for (char& c : out) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return out;
    };
    const std::string q = fold(query);
    if (q.find_first_not_of(' ') == std::string::npos) return;
    std::vector<MenuItem> items;
    int found = 0;
    for (int p : {static_cast<int>(kGames), static_cast<int>(kMedia), static_cast<int>(kHomebrew), static_cast<int>(kNintendo)}) {
        for (size_t i = 0; i < m_pages[p].size(); ++i) {
            const Game& g = m_pages[p][i];
            if (fold(g.name).find(q) == std::string::npos) continue;
            if (++found > 40) break;
            const int page = p, at = static_cast<int>(i);
            MenuItem row = action(g.name, [this, page, at]() {
                goToPage(page);
                m_sels[page] = std::clamp(at, 0, std::max(0, static_cast<int>(m_pages[page].size()) - 1));
                m_zone = Zone::Cards;
            });
            row.detail = pageName(p) + (g.playSeconds ? " · " + std::to_string(g.playSeconds / 3600) + " h de jeu" : std::string());
            row.glyph = p == kGames ? "play" : p == kMedia ? "music" : "dots";
            items.push_back(std::move(row));
        }
    }
    if (items.empty()) {
        say("Rien ne correspond à « " + query + " ».");
        return;
    }
    items.push_back(action("Nouvelle recherche", [this, query]() { openHnpSearch(query); }));
    openMenu(std::to_string(std::min(found, 40)) + " résultat(s) pour « " + query + " »", std::move(items));
}

void CardsActivity::hnpKeyboard() {
    if (!m_hnpKbdReady) {
        hidInitializeKeyboard();
        m_hnpKbdReady = true;
        return;
    }
    HidKeyboardState st{};
    if (hidGetKeyboardStates(&st, 1) == 0) return;
    // Une lettre ou un chiffre qui vient d'etre enfonce ouvre la recherche avec ce caractere.
    char first = 0;
    for (int k = HidKeyboardKey_A; k <= HidKeyboardKey_D0 && !first; ++k) {
        const std::uint64_t bit = 1ULL << (k % 64);
        const bool now = (st.keys[k / 64] & bit) != 0, before = (m_hnpKbdPrev[k / 64] & bit) != 0;
        if (now && !before)
            first = k <= HidKeyboardKey_Z ? static_cast<char>('a' + (k - HidKeyboardKey_A))
                                          : static_cast<char>(k == HidKeyboardKey_D0 ? '0' : '1' + (k - HidKeyboardKey_D1));
    }
    for (int w = 0; w < 4; ++w) m_hnpKbdPrev[w] = st.keys[w];
    if (first) openHnpSearch(std::string(1, first));
}

// ---------------------------------------------------------------- scenes

void CardsActivity::openHnpScenes() {
    const hnp::Scenes sc = loadScenes();
    std::vector<MenuItem> items;
    const bool clk = hnp::sysclkRunning();
    items.push_back(info(clk ? "sys-clk actif" : "sys-clk absent : luminosité seulement", [] { return std::string(); }));
    for (const hnp::Scene& s : sc.list()) {
        const std::string sid = s.id;
        MenuItem row = action(s.name, [this, sid]() {
            hnpRun("Scène en cours d'application...", "", [sid]() {
                hnp::Scenes x = loadScenes();
                const hnp::Scene* one = x.find(sid);
                if (!one) return std::string("Scène introuvable.");
                const std::string msg = hnp::applyScene(*one);
                x.active = sid;
                saveScenes(x);
                return msg;
            }, false);
        });
        if (s.id == sc.active) row.value = [] { return std::string("Active"); };
        std::string d;
        if (s.cpuMHz) d += "CPU " + std::to_string(s.cpuMHz) + " MHz";
        if (s.gpuMHz) d += (d.empty() ? "" : ", ") + std::string("GPU ") + std::to_string(s.gpuMHz) + " MHz";
        if (s.brightness >= 0) d += (d.empty() ? "" : ", ") + std::string("luminosité ") + std::to_string(static_cast<int>(s.brightness * 100 + 0.5)) + " %";
        if (s.colorTempK) d += (d.empty() ? "" : ", ") + std::to_string(s.colorTempK) + " K";
        row.detail = d.empty() ? "Réglages du jeu" : d;
        row.glyph = "sun";
        items.push_back(std::move(row));
    }
    MenuItem none = action("Aucune (profils sys-clk normaux)", [this]() {
        hnpRun("Retour aux réglages normaux...", "", []() {
            hnp::Scenes x = loadScenes();
            x.active.clear();
            saveScenes(x);
            return hnp::clearScene();
        }, false);
    });
    none.glyph = "restart";
    items.push_back(std::move(none));
    items.push_back(action("Retour", [this]() { openHnp(); }));
    openMenu("Scènes", std::move(items));
}

// ---------------------------------------------------------------- PC

void CardsActivity::openHnpPc() {
    const hnp::PcLink pc = pcLink();
    std::vector<MenuItem> items;
    if (!pc.configured()) {
        items.push_back(action("Comment relier le PC", [this]() {
            openReader("Relier le PC", "Sur le PC : lance pc_agent (lancer_pc_agent.ps1), puis outils\\installer_sur_sd.ps1 "
                                       "avec la carte SD dans le PC. Il remplit sd:/config/homenxplus/pc_agent.ini "
                                       "(adresse, port, jeton). La Switch et le PC doivent être sur le même Wi-Fi.", "news");
        }));
    } else {
        MenuItem test = action("Tester la connexion", [this, pc]() {
            hnpRun("Recherche du PC...", "", [pc]() {
                return hnp::pingPc(pc) ? std::string("PC connecté (") + pc.ip + ")."
                                       : std::string("Le PC ne répond pas : allume-le et lance pc_agent.");
            }, false);
        });
        test.glyph = "wifi";
        items.push_back(std::move(test));
        if (!pc.mac.empty()) {
            MenuItem wake = action("Réveiller le PC", [this, pc]() {
                hnpRun("Signal de réveil...", "", [pc]() { return hnp::wakePc(pc); }, false);
            });
            wake.glyph = "power";
            items.push_back(std::move(wake));
        }
        MenuItem send = action("Envoyer un message au PC", [this, pc]() {
            askText("Message pour le PC", "Il arrive dans Documents\\Switch_Sauvegardes\\boite", "", 500, [this, pc](const std::string& t) {
                if (t.find_first_not_of(" \r\n\t") == std::string::npos) return;
                hnpRun("Envoi…", "", [pc, t]() { return hnp::sendToPc(pc, t); }, false);
            });
        });
        send.glyph = "news";
        items.push_back(std::move(send));
        MenuItem recv = action("Lire les messages du PC", [this, pc]() {
            hnpRun("Lecture de la boîte aux lettres...", "Messages du PC", [pc]() {
                int n = 0;
                const std::string all = hnp::readFromPc(pc, n);
                return n ? std::to_string(n) + " message(s)\n\n" + all : std::string("Aucun nouveau message (ou PC injoignable).");
            }, true);
        });
        recv.glyph = "bell";
        items.push_back(std::move(recv));
        MenuItem caps = action("Envoyer mes captures au PC", [this, pc]() {
            hnpRun("Envoi des captures...", "", [pc]() { return hnp::sendCapturesToPc(pc, 30); }, false);
        });
        caps.detail = "Les 30 dernières pas encore envoyées (l'Album n'est pas modifié)";
        caps.glyph = "sys_album";
        items.push_back(std::move(caps));
    }
    items.push_back(action("Retour", [this]() { openHnp(); }));
    openMenu("PC", std::move(items));
}

// ---------------------------------------------------------------- streaming
// La Switch n'a pas d'appli Netflix, Disney+ ni Prime Video, et son navigateur n'a pas le DRM
// (Widevine) que ces sites exigent : le PC ouvre le site (pc_agent, liste fermee), puis
// Moonlight affiche l'ecran du PC sur la Switch, manette en main.

void CardsActivity::hnpWatch(const std::string& service, const std::string& name) {
    const hnp::PcLink pc = pcLink();
    if (!pc.configured()) {
        openReader(name, "Pour regarder " + name + " sur la Switch, il faut le lien avec le PC : lance pc_agent sur le PC, "
                         "puis outils\\installer_sur_sd.ps1 (carte SD dans le PC). Ensuite : PC allumé ou réveillable "
                         "(Wake-on-LAN), Sunshine ou GeForce Experience pour Moonlight.", "wifi");
        return;
    }
    hnpRun("Ouverture de " + name + " sur le PC (réveil si besoin, jusqu'à 1 min)...", name, [pc, service]() {
        std::string msg;
        const bool ok = hnp::openOnPc(pc, service, msg);
        return std::string(ok ? "1" : "0") + msg;
    }, false, [this, name](const std::string& r) {
        if (r.empty() || r[0] != '1') {
            say(r.size() > 1 ? r.substr(1) : std::string("Échec."));
            return;
        }
        std::string moon = "sdmc:/switch/Moonlight/Moonlight-Switch.nro";
        std::error_code ec;
        if (!std::filesystem::exists(moon, ec)) moon = findNro("moonlight");
        if (moon.empty()) {
            say(name + " est ouvert sur le PC. Moonlight n'est pas installé sur la Switch.");
            return;
        }
        say(name + " est ouvert sur le PC : Moonlight démarre, choisis Bureau (Desktop).");
        openNro(moon);
    });
}

void CardsActivity::openHnpStreaming() {
    std::vector<MenuItem> items;
    for (const hnp::StreamingService& s : hnp::kStreamingServices) {
        const std::string sid = s.id, label = s.name;
        MenuItem row = action(label, [this, sid, label]() { hnpWatch(sid, label); });
        row.detail = std::string(s.about) + " · sur le PC, affiché par Moonlight";
        row.glyph = s.music ? "music" : "play";
        items.push_back(std::move(row));
    }
#ifdef SWITCHU_MENU
    MenuItem remote = action("Télécommande Spotify", [this]() { openMusic(); });
    remote.detail = "Contrôle Spotify sur ton téléphone ou ton PC (Spotify Connect)";
    remote.glyph = "playlist";
    items.push_back(std::move(remote));
#endif
    MenuItem why = action("Pourquoi passer par le PC ?", [this]() {
        openReader("Netflix, Disney+, Prime Video sur Switch",
                   "Ces services n'ont jamais sorti d'appli pour la Switch, et le navigateur de la console ne gère pas "
                   "leur protection anti-copie (DRM Widevine). La seule méthode fiable et légale : le site tourne sur ton PC "
                   "et Moonlight affiche son écran sur la Switch.\n\n"
                   "À faire une fois sur le PC :\n"
                   "1. Sunshine (ou GeForce Experience) pour Moonlight, et Moonlight appairé avec la Switch.\n"
                   "2. pc_agent au démarrage de Windows (lancer_pc_agent.ps1).\n"
                   "3. Dans le navigateur du PC : connexion à Netflix, Disney+, Prime Video une fois.\n"
                   "4. Si l'image est noire dans Moonlight : dans le navigateur du PC, désactive l'accélération "
                   "matérielle (Paramètres > Système), car le DRM masque la vidéo à la capture.\n\n"
                   "Ensuite ici : un appui réveille le PC si besoin, ouvre le site, puis lance Moonlight.\n\n"
                   "Spotify : la Télécommande contrôle la musique qui joue sur ton téléphone ou ton PC ; "
                   "pour l'entendre sur la Switch, passe aussi par Moonlight.", "news");
    });
    why.glyph = "news";
    items.push_back(std::move(why));
    items.push_back(action("Retour", [this]() { openHnp(); }));
    openMenu("Films, séries et musique par le PC", std::move(items));
}

// ---------------------------------------------------------------- le panneau

void CardsActivity::openHnp() {
    m_dotsOpen = false;
    std::vector<MenuItem> items;
    MenuItem style = action("Style d'affichage", [this]() { openHnpStyle(); });
    style.value = [] { return std::string("Cartes"); };
    style.glyph = "copy";
    items.push_back(std::move(style));
    MenuItem cl = action("Claude", [this]() { openHnpClaude(); });
    cl.glyph = "news";
    items.push_back(std::move(cl));
    MenuItem find = action("Rechercher (Y, ou tape au clavier USB)", [this]() { openHnpSearch(); });
    find.glyph = "tag";
    items.push_back(std::move(find));
    if (const Game* g = current(); g && g->path.empty() && g->action.empty() && !g->homebrew) {
        const std::uint64_t id = g->id;
        const std::string name = g->name;
        MenuItem sheet = action("Fiche : " + name, [this, id, name]() { openHnpGame(id, name); });
        sheet.glyph = "trophy";
        items.push_back(std::move(sheet));
    }
    MenuItem tv = action("Médias : films, séries, musique, Discord", [this]() { goToPage(kMedia); });
    tv.detail = "Netflix, Disney+, Prime Video, Spotify (par le PC), Switchfin, Moonlight...";
    tv.glyph = "play";
    items.push_back(std::move(tv));
    MenuItem tvHelp = action("Netflix, Disney+, Prime Video : comment ça marche", [this]() { openHnpStreaming(); });
    tvHelp.glyph = "news";
    items.push_back(std::move(tvHelp));
    MenuItem sc = action("Scènes (fréquences, luminosité)", [this]() { openHnpScenes(); });
    sc.value = [] {
        const hnp::Scenes s = loadScenes();
        const hnp::Scene* a = s.find(s.active);
        return a ? a->name : std::string();
    };
    sc.glyph = "sun";
    items.push_back(std::move(sc));
    MenuItem pc = action("PC (réveil, messages, sauvegardes)", [this]() { openHnpPc(); });
    pc.glyph = "wifi";
    items.push_back(std::move(pc));
    if (hasKey()) {
        MenuItem assist = action("Ranger mes jeux avec Claude", [this]() { hnpAssistSort(); });
        assist.detail = "Claude propose des pages par genre ; tu acceptes ou non";
        assist.glyph = "copy";
        items.push_back(std::move(assist));
    }
    MenuItem pcPlay = action("Jouer aux jeux du PC", [this]() { hnpPlayPc(); });
    pcPlay.detail = "Réveille le PC si besoin, puis Moonlight";
    pcPlay.glyph = "power";
    items.push_back(std::move(pcPlay));
    MenuItem smart = action("Pages intelligentes", [this]() {
        // Recents, Plus joues, Jamais lances : des pages perso remplies d'apres l'historique de jeu.
        std::map<std::string, hnp::Item> all;
        for (const Game& g : m_pages[kGames]) {
            if (!g.path.empty() || !g.action.empty() || g.homebrew) continue;
            hnp::Item it;
            it.id = idText(g.id);
            it.title = g.name;
            it.playMinutes = static_cast<std::int64_t>(g.playSeconds / 60);
            it.lastPlayed = static_cast<std::int64_t>(g.lastPlayed);
            it.launches = static_cast<int>(g.launches);
            all[it.id] = it;
        }
        const std::pair<const char*, hnp::Smart> kinds[] = {
            {"Récents", hnp::Smart::Recent}, {"Plus joués", hnp::Smart::MostPlayed}, {"Jamais lancés", hnp::Smart::NeverLaunched}};
        int made = 0, updated = 0;
        for (const auto& [title, kind] : kinds) {
            std::vector<std::string> ids = hnp::smartFolder(kind, all, {}, 30);
            const std::string old = title == std::string("Récents") ? "Recents" : title == std::string("Plus joués") ? "Plus joues" : "Jamais lances";
            auto page = std::find_if(m_settings.customPages.begin(), m_settings.customPages.end(),
                                     [&](const CustomPage& p) { return p.name == title || p.name == old; });
            if (page != m_settings.customPages.end()) page->name = title;
            if (page != m_settings.customPages.end()) {
                page->titles = std::move(ids);
                ++updated;
            } else if (m_settings.customPages.size() < Settings::kMaxCustomPages) {
                m_settings.customPages.push_back({title, std::move(ids)});
                ++made;
            }
        }
        m_settings.normalizePages();
        m_settings.save();
        rebuildCustomPages();
        m_audio.play("toggle_on");
        say(made || updated ? "Pages Récents, Plus joués et Jamais lancés à jour : ZL / ZR pour y aller."
                            : "Plus de place pour de nouvelles pages (8 au plus).");
    });
    smart.detail = "Récents, Plus joués, Jamais lancés : mises à jour à chaque appui";
    smart.glyph = "sort";
    items.push_back(std::move(smart));
    MenuItem health = action("Batterie et santé", [this]() {
        const hnp::BatteryStatus b = hnp::readBattery();
        std::string t = "Batterie : " + (b.percent < 0 ? std::string("inconnue") : std::to_string(b.percent) + " %") +
                        (b.charging ? " (en charge)" : "") + "\n";
        if (b.percent >= 0 && !b.charging && b.percent <= 15) {
            hnp::BatteryGuard g;
            t += hnp::BatteryGuard::message(g.update(b.percent, false)) + "\n";
        } else {
            t += "Avec l'AutoRCM, une batterie vide oblige à réinjecter Hekate depuis le PC : recharge sous 15 %.\n";
        }
        s64 freeB = 0, totalB = 0;
        FsFileSystem* sd = fsdevGetDeviceFileSystem("sdmc");
        if (sd) {
            fsFsGetFreeSpace(sd, "/", &freeB);
            fsFsGetTotalSpace(sd, "/", &totalB);
        }
        char sp[96];
        std::snprintf(sp, sizeof sp, "\nCarte SD : %.1f Go libres sur %.1f Go%s\n", freeB / 1e9, totalB / 1e9,
                      freeB && freeB < 5'000'000'000LL ? " (moins de 5 Go : fais de la place)" : "");
        t += sp;
        t += std::string("\nsys-clk : ") + (hnp::sysclkRunning() ? "actif" : "absent") + "\n";
        t += std::string("Mode sans échec : ") + (hnp::g_safeMode ? "ACTIF (le menu a planté 3 fois de suite)" : "inactif") + "\n";
        openReader("Batterie et santé", t, "thermo");
    });
    health.glyph = "thermo";
    items.push_back(std::move(health));
    MenuItem about = action("À propos de HomeNX+", [this]() {
        openReader("À propos", std::string(kVersion) + "\n\nTon menu HOME amélioré : apparence Cartes ou Wii U, Claude integre, "
                   "scènes, fiche de jeu (notes, checklist), lien avec le PC, overlay en jeu.\n\n"
                   "Base : HomeNX (ad3333m), SwitchU (PoloNX), CardsNX. Licence GPL-3.0.\n"
                   "Overlay : libtesla (GPL-2.0 ou plus). Claude : API Anthropic directe.\n\n"
                   "Secours : si le menu ne démarre plus, lance secours_menu_home.ps1 sur le PC (ou le script Linux).",
                   "dots");
    });
    about.glyph = "dots";
    items.push_back(std::move(about));
    items.push_back(action("Fermer", [] {}));
    openMenu(kVersion, std::move(items));
}

// ---------------------------------------------------------------- l'onglet des reglages

CardsActivity::Section CardsActivity::hnpSection() {
    Section s{"HomeNX+", "trophy", {}};
    MenuItem open = action("Ouvrir le panneau HomeNX+", [this]() { openHnp(); });
    open.detail = "Aussi dans les trois points en haut à gauche";
    s.rows.push_back(std::move(open));
    MenuItem style = action("Style d'affichage", [this]() { openHnpStyle(); });
    style.value = [] { return std::string("Cartes"); };
    style.detail = "Cartes (celle-ci) ou Wii U (HomeNX) : le menu redémarre";
    s.rows.push_back(std::move(style));
    MenuItem cl = action("Claude", [this]() { openHnpClaude(); });
    cl.value = [] { return hasKey() ? monthSpend() : std::string("Pas de clé"); };
    cl.detail = "Questions, aide sur une capture, traduction";
    s.rows.push_back(std::move(cl));
    MenuItem tv = action("Médias", [this]() { m_sheetOpen = false; goToPage(kMedia); });
    tv.detail = "La page des films, séries, musique et Discord (Netflix & co : par le PC)";
    s.rows.push_back(std::move(tv));
    MenuItem sc = action("Scènes", [this]() { openHnpScenes(); });
    sc.detail = "Nuit, Batterie, Perf max : fréquences et luminosité";
    s.rows.push_back(std::move(sc));
    MenuItem pc = action("PC", [this]() { openHnpPc(); });
    pc.value = [] { return pcLink().configured() ? std::string("Configuré") : std::string("Non relié"); };
    s.rows.push_back(std::move(pc));
    s.rows.push_back(info("Mode sans échec", [] { return std::string(hnp::g_safeMode ? "ACTIF" : "Inactif"); }));
    s.rows.push_back(info("Version", [] { return std::string(kVersion); }));
    return s;
}

} // namespace cards

#endif // HNP_PLUS
