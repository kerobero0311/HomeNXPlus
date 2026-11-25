// HomeNX+ : ses ajouts dans l'apparence Cartes (CardsNX).
//  - trois points > HomeNX+ et Reglages > HomeNX+ : Claude, scenes, PC, rangement, sante ;
//  - trois points > Style : passer a l'apparence Wii U (et inversement depuis celle-ci) ;
//  - + sur un jeu > Fiche HomeNX+ : notes, checklist, Claude, saves.
// Les taches longues (reseau, Claude) tournent sur un fil a part : le menu ne gele jamais.
// Compile seulement dans le menu HomeNX+ (HNP_PLUS, xmake.lua), pas dans CardsNX seul.
#ifdef HNP_PLUS

#include "CardsActivity.hpp"

#include "hnp/claude_tasks.hpp"
#include "hnp/layout.hpp"
#include "hnp/notes.hpp"
#include "hnp/pclink.hpp"
#include "hnp/safemode.hpp"
#include "hnp/scenes.hpp"
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
constexpr const char* kVersion    = "HomeNX+ 0.9.0";

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

const char* kNoKey = "Pas de cle API : cree-la sur console.anthropic.com (avec une limite de depenses), "
                     "puis mets-la seule dans sd:/config/homenxplus/api_key.txt.";

} // namespace

// ---------------------------------------------------------------- taches de fond

void CardsActivity::hnpRun(const std::string& busy, const std::string& title, std::function<std::string()> job, bool reader,
                           std::function<void(const std::string&)> done) {
    if (m_hnpJob.valid()) {
        say("Une action HomeNX+ est deja en cours, patiente un instant.");
        return;
    }
    m_hnpJobTitle = title;
    m_hnpJobReader = reader;
    m_hnpDone = std::move(done);
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
    if (!m_hnpJob.valid() || m_hnpJob.wait_for(std::chrono::seconds(0)) != std::future_status::ready) return;
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
        say("Ferme d'abord le jeu ou l'app ouvert (+ > Fermer) : le menu doit redemarrer.");
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
    say(toCards ? "Apparence Cartes : le menu redemarre..." : "Apparence Wii U : le menu redemarre...");
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
    MenuItem wiiu = action("Passer a l'apparence Wii U (HomeNX)", [this]() { hnpSwitchStyle(false); });
    wiiu.detail = "Grille de jeux, dossiers, reglages complets. Le menu redemarre (2 s).";
    wiiu.glyph = "copy";
    items.push_back(std::move(wiiu));
    MenuItem look = action("Couleurs, verre, fond, animations...", [this]() {
        openSettings();
        for (size_t i = 0; i < m_sheet.size(); ++i)
            if (m_sheet[i].name == "Look") m_sheetSec = static_cast<int>(i);
    });
    look.detail = "Les reglages d'apparence des Cartes";
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
    items.push_back(info("Cle API", [key] { return std::string(key ? "Presente" : "Absente"); }));
    items.push_back(info("Depense du mois", [] { return monthSpend(); }));
    if (!key) {
        items.push_back(action("Comment l'installer", [this]() { openReader("Cle API Claude", kNoKey, "news"); }));
    } else {
        MenuItem ask = action("Poser une question", [this]() {
            askText("Question pour Claude", "Ecris ta question", "", 500, [this](const std::string& q) {
                if (q.find_first_not_of(" \r\n\t") == std::string::npos) return;
                hnpRun("Claude reflechit...", "Claude", [q]() {
                    std::string out;
                    hnp::askClaudeOnce(q, out, 1500);
                    return "Toi : " + q + "\n\n" + out;
                }, true);
            });
        });
        ask.detail = "La reponse s'affiche ici (quelques centimes au plus)";
        ask.glyph = "news";
        items.push_back(std::move(ask));
        MenuItem help = action("Que faire ici ? (derniere capture)", [this]() {
            openMenu("Envoyer ta derniere capture a Claude ?", {
                info("Seule la derniere capture de l'Album part", [] { return std::string(); }),
                action("Oui, envoyer", [this]() {
                    hnpRun("Claude regarde ta capture...", "Que faire ici ?", []() {
                        std::vector<std::uint8_t> jpeg;
                        std::string when, out;
                        if (!hnp::lastScreenshot(jpeg, when)) return std::string("Aucune capture d'ecran dans l'Album : appuie sur le bouton Capture en jeu.");
                        hnp::askClaudeOnce("Voici une capture d'ecran de mon jeu Nintendo Switch. Explique simplement "
                                           "ce que je dois faire ici pour avancer, en 5 etapes au plus, en francais.",
                                           out, 1200, &jpeg);
                        return "Capture du " + when + "\n\n" + out;
                    }, true);
                }),
                action("Annuler", [] {}),
            });
        });
        help.glyph = "sys_album";
        items.push_back(std::move(help));
        MenuItem trad = action("Traduire ma derniere capture", [this]() {
            openMenu("Envoyer ta derniere capture a Claude ?", {
                info("Claude traduit son texte en francais", [] { return std::string(); }),
                action("Oui, traduire", [this]() {
                    hnpRun("Traduction en cours...", "Traduction", []() {
                        std::vector<std::uint8_t> jpeg;
                        std::string when, out;
                        if (!hnp::lastScreenshot(jpeg, when)) return std::string("Aucune capture d'ecran dans l'Album.");
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
            say("Note enregistree.");
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
                say("Notes effacees.");
            }, true),
            action("Annuler", [] {}),
        });
    }, true);
    clear.glyph = "trash";
    items.push_back(std::move(clear));
    MenuItem see = action(cl.items.empty() ? "Checklist de completion" :
                          "Checklist : " + std::to_string(cl.percent()) + " %", [this, tid, name]() {
        openReader("Checklist : " + name, checklistText(loadChecklist(tid)), "trophy");
    });
    see.glyph = "trophy";
    items.push_back(std::move(see));
    if (hasKey()) {
        MenuItem gen = action(cl.items.empty() ? "Creer la checklist avec Claude" : "Refaire la checklist avec Claude",
                              [this, tid, name]() {
            hnpRun("Claude prepare la checklist...", "Checklist : " + name, [tid, name]() {
                std::string reply;
                if (!hnp::askClaudeOnce(hnp::checklistPrompt(name), reply)) return reply;
                hnp::Checklist c = hnp::Checklist::fromText(tid, reply);
                if (c.items.empty() || (c.items.size() == 1 && c.items[0].label == "Jeu inconnu"))
                    return std::string("Claude ne connait pas assez ce jeu pour une checklist fiable.");
                c.source = "claude";
                saveChecklist(c);
                return std::to_string(c.items.size()) + " objectifs enregistres.\n\n" + checklistText(c);
            }, true);
        });
        gen.detail = "Quelques centimes au plus";
        gen.glyph = "news";
        items.push_back(std::move(gen));
        MenuItem ask = action("Demander a Claude sur ce jeu", [this, name]() {
            askText("Question sur " + name, "Ex. : ou trouver la cle du donjon ?", "", 500, [this, name](const std::string& q) {
                if (q.find_first_not_of(" \r\n\t") == std::string::npos) return;
                hnpRun("Claude reflechit...", name, [q, name]() {
                    std::string out;
                    hnp::askClaudeOnce("Je joue a \"" + name + "\" sur Nintendo Switch. " + q +
                                       " Reponds simplement, sans gacher la suite si ce n'est pas demande.", out, 1500);
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
        saves.detail = "Lecture seule, vers Documents\\Switch_Sauvegardes (pc_agent lance)";
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
        MenuItem scene = choice("Scene de ce jeu", [cur] { return cur; }, [this, tid, id, name](int) {
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
            openMenu("Scene pour " + name, std::move(pick));
        }, "Frequences et luminosite pour ce jeu");
        items.push_back(std::move(scene));
    }
    items.push_back(action("Fermer", [] {}));
    openMenu(name, std::move(items));
}

// ---------------------------------------------------------------- scenes

void CardsActivity::openHnpScenes() {
    const hnp::Scenes sc = loadScenes();
    std::vector<MenuItem> items;
    const bool clk = hnp::sysclkRunning();
    items.push_back(info(clk ? "sys-clk actif" : "sys-clk absent : luminosite seulement", [] { return std::string(); }));
    for (const hnp::Scene& s : sc.list()) {
        const std::string sid = s.id;
        MenuItem row = action(s.name, [this, sid]() {
            hnpRun("Scene en cours d'application...", "", [sid]() {
                hnp::Scenes x = loadScenes();
                const hnp::Scene* one = x.find(sid);
                if (!one) return std::string("Scene introuvable.");
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
        if (s.brightness >= 0) d += (d.empty() ? "" : ", ") + std::string("luminosite ") + std::to_string(static_cast<int>(s.brightness * 100 + 0.5)) + " %";
        if (s.colorTempK) d += (d.empty() ? "" : ", ") + std::to_string(s.colorTempK) + " K";
        row.detail = d.empty() ? "Reglages du jeu" : d;
        row.glyph = "sun";
        items.push_back(std::move(row));
    }
    MenuItem none = action("Aucune (profils sys-clk normaux)", [this]() {
        hnpRun("Retour aux reglages normaux...", "", []() {
            hnp::Scenes x = loadScenes();
            x.active.clear();
            saveScenes(x);
            return hnp::clearScene();
        }, false);
    });
    none.glyph = "restart";
    items.push_back(std::move(none));
    items.push_back(action("Retour", [this]() { openHnp(); }));
    openMenu("Scenes", std::move(items));
}

// ---------------------------------------------------------------- PC

void CardsActivity::openHnpPc() {
    const hnp::PcLink pc = pcLink();
    std::vector<MenuItem> items;
    if (!pc.configured()) {
        items.push_back(action("Comment relier le PC", [this]() {
            openReader("Relier le PC", "Sur le PC : lance pc_agent (lancer_pc_agent.ps1), puis outils\\installer_sur_sd.ps1 "
                                       "avec la carte SD dans le PC. Il remplit sd:/config/homenxplus/pc_agent.ini "
                                       "(adresse, port, jeton). La Switch et le PC doivent etre sur le meme Wi-Fi.", "news");
        }));
    } else {
        MenuItem test = action("Tester la connexion", [this, pc]() {
            hnpRun("Recherche du PC...", "", [pc]() {
                return hnp::pingPc(pc) ? std::string("PC connecte (") + pc.ip + ")."
                                       : std::string("Le PC ne repond pas : allume-le et lance pc_agent.");
            }, false);
        });
        test.glyph = "wifi";
        items.push_back(std::move(test));
        if (!pc.mac.empty()) {
            MenuItem wake = action("Reveiller le PC", [this, pc]() {
                hnpRun("Signal de reveil...", "", [pc]() { return hnp::wakePc(pc); }, false);
            });
            wake.glyph = "power";
            items.push_back(std::move(wake));
        }
        MenuItem send = action("Envoyer un message au PC", [this, pc]() {
            askText("Message pour le PC", "Il arrive dans Documents\\Switch_Sauvegardes\\boite", "", 500, [this, pc](const std::string& t) {
                if (t.find_first_not_of(" \r\n\t") == std::string::npos) return;
                hnpRun("Envoi...", "", [pc, t]() { return hnp::sendToPc(pc, t); }, false);
            });
        });
        send.glyph = "news";
        items.push_back(std::move(send));
        MenuItem recv = action("Lire les messages du PC", [this, pc]() {
            hnpRun("Lecture de la boite aux lettres...", "Messages du PC", [pc]() {
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
        caps.detail = "Les 30 dernieres pas encore envoyees (l'Album n'est pas modifie)";
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
                         "puis outils\\installer_sur_sd.ps1 (carte SD dans le PC). Ensuite : PC allume ou reveillable "
                         "(Wake-on-LAN), Sunshine ou GeForce Experience pour Moonlight.", "wifi");
        return;
    }
    hnpRun("Ouverture de " + name + " sur le PC (reveil si besoin, jusqu'a 1 min)...", name, [pc, service]() {
        std::string msg;
        const bool ok = hnp::openOnPc(pc, service, msg);
        return std::string(ok ? "1" : "0") + msg;
    }, false, [this, name](const std::string& r) {
        if (r.empty() || r[0] != '1') {
            say(r.size() > 1 ? r.substr(1) : std::string("Echec."));
            return;
        }
        std::string moon = "sdmc:/switch/Moonlight/Moonlight-Switch.nro";
        std::error_code ec;
        if (!std::filesystem::exists(moon, ec)) moon = findNro("moonlight");
        if (moon.empty()) {
            say(name + " est ouvert sur le PC. Moonlight n'est pas installe sur la Switch.");
            return;
        }
        say(name + " est ouvert sur le PC : Moonlight demarre, choisis Bureau (Desktop).");
        openNro(moon);
    });
}

void CardsActivity::openHnpStreaming() {
    std::vector<MenuItem> items;
    const std::pair<const char*, const char*> kServices[] = {
        {"netflix", "Netflix"}, {"disney", "Disney+"}, {"prime", "Prime Video"}, {"spotify", "Spotify"}};
    for (const auto& [id, name] : kServices) {
        const std::string sid = id, label = name;
        MenuItem row = action(label, [this, sid, label]() { hnpWatch(sid, label); });
        row.detail = sid == "spotify" ? "Ouvert sur le PC, le son et l'image arrivent par Moonlight"
                                      : "Sur le PC, regarde sur la Switch avec Moonlight";
        row.glyph = sid == "spotify" ? "music" : "play";
        items.push_back(std::move(row));
    }
#ifdef SWITCHU_MENU
    MenuItem remote = action("Telecommande Spotify", [this]() { openMusic(); });
    remote.detail = "Controle Spotify sur ton telephone ou ton PC (Spotify Connect)";
    remote.glyph = "playlist";
    items.push_back(std::move(remote));
#endif
    MenuItem why = action("Pourquoi passer par le PC ?", [this]() {
        openReader("Netflix, Disney+, Prime Video sur Switch",
                   "Ces services n'ont jamais sorti d'appli pour la Switch, et le navigateur de la console ne gere pas "
                   "leur protection anti-copie (DRM Widevine). La seule methode fiable et legale : le site tourne sur ton PC "
                   "et Moonlight affiche son ecran sur la Switch.\n\n"
                   "A faire une fois sur le PC :\n"
                   "1. Sunshine (ou GeForce Experience) pour Moonlight, et Moonlight appaire avec la Switch.\n"
                   "2. pc_agent au demarrage de Windows (lancer_pc_agent.ps1).\n"
                   "3. Dans le navigateur du PC : connexion a Netflix, Disney+, Prime Video une fois.\n"
                   "4. Si l'image est noire dans Moonlight : dans le navigateur du PC, desactive l'acceleration "
                   "materielle (Parametres > Systeme), car le DRM masque la video a la capture.\n\n"
                   "Ensuite ici : un appui reveille le PC si besoin, ouvre le site, puis lance Moonlight.\n\n"
                   "Spotify : la Telecommande controle la musique qui joue sur ton telephone ou ton PC ; "
                   "pour l'entendre sur la Switch, passe aussi par Moonlight.", "news");
    });
    why.glyph = "news";
    items.push_back(std::move(why));
    items.push_back(action("Retour", [this]() { openHnp(); }));
    openMenu("Films, series et musique", std::move(items));
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
    if (const Game* g = current(); g && g->path.empty() && g->action.empty() && !g->homebrew) {
        const std::uint64_t id = g->id;
        const std::string name = g->name;
        MenuItem sheet = action("Fiche : " + name, [this, id, name]() { openHnpGame(id, name); });
        sheet.glyph = "trophy";
        items.push_back(std::move(sheet));
    }
    MenuItem tv = action("Netflix, Disney+, Prime Video, Spotify", [this]() { openHnpStreaming(); });
    tv.glyph = "play";
    items.push_back(std::move(tv));
    MenuItem sc = action("Scenes (frequences, luminosite)", [this]() { openHnpScenes(); });
    sc.value = [] {
        const hnp::Scenes s = loadScenes();
        const hnp::Scene* a = s.find(s.active);
        return a ? a->name : std::string();
    };
    sc.glyph = "sun";
    items.push_back(std::move(sc));
    MenuItem pc = action("PC (reveil, messages, sauvegardes)", [this]() { openHnpPc(); });
    pc.glyph = "wifi";
    items.push_back(std::move(pc));
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
            {"Recents", hnp::Smart::Recent}, {"Plus joues", hnp::Smart::MostPlayed}, {"Jamais lances", hnp::Smart::NeverLaunched}};
        int made = 0, updated = 0;
        for (const auto& [title, kind] : kinds) {
            std::vector<std::string> ids = hnp::smartFolder(kind, all, {}, 30);
            auto page = std::find_if(m_settings.customPages.begin(), m_settings.customPages.end(),
                                     [&](const CustomPage& p) { return p.name == title; });
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
        say(made || updated ? "Pages Recents, Plus joues et Jamais lances a jour : ZL / ZR pour y aller."
                            : "Plus de place pour de nouvelles pages (8 au plus).");
    });
    smart.detail = "Recents, Plus joues, Jamais lances : mises a jour a chaque appui";
    smart.glyph = "sort";
    items.push_back(std::move(smart));
    MenuItem health = action("Batterie et sante", [this]() {
        const hnp::BatteryStatus b = hnp::readBattery();
        std::string t = "Batterie : " + (b.percent < 0 ? std::string("inconnue") : std::to_string(b.percent) + " %") +
                        (b.charging ? " (en charge)" : "") + "\n";
        if (b.percent >= 0 && !b.charging && b.percent <= 15) {
            hnp::BatteryGuard g;
            t += hnp::BatteryGuard::message(g.update(b.percent, false)) + "\n";
        } else {
            t += "Avec l'AutoRCM, une batterie vide oblige a reinjecter Hekate depuis le PC : recharge sous 15 %.\n";
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
        t += std::string("Mode sans echec : ") + (hnp::g_safeMode ? "ACTIF (le menu a plante 3 fois de suite)" : "inactif") + "\n";
        openReader("Batterie et sante", t, "thermo");
    });
    health.glyph = "thermo";
    items.push_back(std::move(health));
    MenuItem about = action("A propos de HomeNX+", [this]() {
        openReader("A propos", std::string(kVersion) + "\n\nTon menu HOME ameliore : apparence Cartes ou Wii U, Claude integre, "
                   "scenes, fiche de jeu (notes, checklist), lien avec le PC, overlay en jeu.\n\n"
                   "Base : HomeNX (ad3333m), SwitchU (PoloNX), CardsNX. Licence GPL-3.0.\n"
                   "Overlay : libtesla (GPL-2.0 ou plus). Claude : API Anthropic directe.\n\n"
                   "Secours : si le menu ne demarre plus, lance secours_menu_home.ps1 sur le PC (ou le script Linux).",
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
    open.detail = "Aussi dans les trois points en haut a gauche";
    s.rows.push_back(std::move(open));
    MenuItem style = action("Style d'affichage", [this]() { openHnpStyle(); });
    style.value = [] { return std::string("Cartes"); };
    style.detail = "Cartes (celle-ci) ou Wii U (HomeNX) : le menu redemarre";
    s.rows.push_back(std::move(style));
    MenuItem cl = action("Claude", [this]() { openHnpClaude(); });
    cl.value = [] { return hasKey() ? monthSpend() : std::string("Pas de cle"); };
    cl.detail = "Questions, aide sur une capture, traduction";
    s.rows.push_back(std::move(cl));
    MenuItem tv = action("Netflix, Disney+, Prime Video, Spotify", [this]() { openHnpStreaming(); });
    tv.detail = "Par le PC et Moonlight (un appui : reveil, site, Moonlight)";
    s.rows.push_back(std::move(tv));
    MenuItem sc = action("Scenes", [this]() { openHnpScenes(); });
    sc.detail = "Nuit, Batterie, Perf max : frequences et luminosite";
    s.rows.push_back(std::move(sc));
    MenuItem pc = action("PC", [this]() { openHnpPc(); });
    pc.value = [] { return pcLink().configured() ? std::string("Configure") : std::string("Non relie"); };
    s.rows.push_back(std::move(pc));
    s.rows.push_back(info("Mode sans echec", [] { return std::string(hnp::g_safeMode ? "ACTIF" : "Inactif"); }));
    s.rows.push_back(info("Version", [] { return std::string(kVersion); }));
    return s;
}

} // namespace cards

#endif // HNP_PLUS
