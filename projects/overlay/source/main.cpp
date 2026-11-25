// HomeNX+ : overlay en jeu (Ultrahand / Tesla), meme combinaison qu'Ultrahand (L + Bas + Stick droit).
// - Batterie (avec le rappel AutoRCM)   - Scenes (sys-clk + luminosite)
// - Notes et checklist du jeu en cours (cocher en jeu ; enregistre sur la carte)
#include <bit>  // libtesla utilise std::bit_cast

#define TESLA_INIT_IMPL
#include <tesla.hpp>

#include "hnp/notes.hpp"
#include "hnp/scenes.hpp"
#include "hnp/switch_actions.hpp"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace {

std::string readAll(const std::string& p) { std::ifstream in(p, std::ios::binary); std::stringstream ss; ss << in.rdbuf(); return ss.str(); }

void writeAll(const std::string& p, const std::string& data) {
    std::error_code ec; std::filesystem::create_directories(std::filesystem::path(p).parent_path(), ec);
    { std::ofstream out(p + ".tmp", std::ios::binary | std::ios::trunc); out << data; }
    std::filesystem::rename(p + ".tmp", p, ec);
}

std::string titleIdHex(std::uint64_t tid) { char b[17]; std::snprintf(b, sizeof b, "%016lX", (unsigned long)tid); return b; }

std::uint64_t runningTitle() {  // jeu en cours (0 = aucun)
    std::uint64_t pid = 0, tid = 0;
    if (R_FAILED(pmdmntGetApplicationProcessId(&pid))) return 0;
    if (R_FAILED(pminfoGetProgramId(&tid, pid))) return 0;
    return (tid >> 56) == 0x01 ? tid & ~0x1FFFULL : tid;
}

hnp::Scenes loadScenes() {
    auto j = nlohmann::json::parse(readAll("sdmc:/homenxplus/scenes.json"), nullptr, false);
    return hnp::Scenes::fromJson(j.is_discarded() ? nlohmann::json() : j);
}

// Coupe un long texte en lignes d'environ 38 caracteres pour l'overlay
std::vector<std::string> wrap(const std::string& text, std::size_t width = 38) {
    std::vector<std::string> out;
    std::istringstream in(text);
    std::string para;
    while (std::getline(in, para)) {
        std::string line, word;
        std::istringstream w(para);
        while (w >> word) {
            if (!line.empty() && line.size() + 1 + word.size() > width) { out.push_back(line); line.clear(); }
            line += (line.empty() ? "" : " ") + word;
        }
        out.push_back(line);
    }
    return out;
}

class HnpGui : public tsl::Gui {
public:
    tsl::elm::Element* createUI() override {
        auto* frame = new tsl::elm::OverlayFrame("HomeNX+", "0.5");
        auto* list = new tsl::elm::List();

        // ---------------------------------------------------- batterie
        list->addItem(new tsl::elm::CategoryHeader("Batterie"));
        const hnp::BatteryStatus b = hnp::readBattery();
        std::string val = b.percent < 0 ? "?" : std::to_string(b.percent) + " %" + (b.charging ? " (charge)" : "");
        auto* bat = new tsl::elm::ListItem(b.percent >= 0 && b.percent <= 15 && !b.charging ? "Batterie faible !" : "Niveau", val);
        list->addItem(bat);
        if (b.percent >= 0 && b.percent <= 15 && !b.charging)
            list->addItem(new tsl::elm::ListItem("AutoRCM : batterie vide = reinjection"));

        // ---------------------------------------------------- scenes
        list->addItem(new tsl::elm::CategoryHeader("Scenes"));
        hnp::Scenes scenes = loadScenes();
        auto addScene = [&](const std::string& id, const std::string& name) {
            auto* it = new tsl::elm::ListItem(name, id == scenes.active || (id.empty() && scenes.active.empty()) ? "active" : "");
            it->setClickListener([id, it](u64 keys) {
                if (!(keys & HidNpadButton_A)) return false;
                hnp::Scenes sc = loadScenes();
                if (id.empty()) { hnp::clearScene(); sc.active.clear(); }
                else if (const hnp::Scene* s = sc.find(id)) { hnp::applyScene(*s); sc.active = id; }
                writeAll("sdmc:/homenxplus/scenes.json", sc.toJson().dump(1));
                it->setValue("appliquee");
                return true;
            });
            list->addItem(it);
        };
        addScene("", "Aucune (profils sys-clk)");
        for (const auto& s : scenes.list()) addScene(s.id, s.name);

        // ---------------------------------------------------- notes du jeu en cours
        const std::uint64_t tid = runningTitle();
        list->addItem(new tsl::elm::CategoryHeader(tid ? "Notes du jeu" : "Notes (aucun jeu lance)"));
        if (tid) {
            const std::string id = titleIdHex(tid);
            auto nj = nlohmann::json::parse(readAll(hnp::notesPath(id)), nullptr, false);
            const hnp::GameNotes notes = hnp::GameNotes::fromJson(nj.is_discarded() ? nlohmann::json() : nj);
            if (notes.text.empty()) list->addItem(new tsl::elm::ListItem("Aucune note (ajoute-les dans HomeNX+)"));
            for (const auto& l : wrap(notes.text)) list->addItem(new tsl::elm::ListItem(l));

            auto cj = nlohmann::json::parse(readAll(hnp::checklistPath(id)), nullptr, false);
            hnp::Checklist cl = hnp::Checklist::fromJson(cj.is_discarded() ? nlohmann::json() : cj);
            if (!cl.items.empty()) {
                list->addItem(new tsl::elm::CategoryHeader("Checklist " + std::to_string(cl.percent()) + " %"));
                for (std::size_t i = 0; i < cl.items.size(); ++i) {
                    auto* t = new tsl::elm::ToggleListItem(cl.items[i].label, cl.items[i].done, "Fait", "A faire");
                    t->setStateChangedListener([id, i](bool on) {
                        auto j = nlohmann::json::parse(readAll(hnp::checklistPath(id)), nullptr, false);
                        hnp::Checklist c = hnp::Checklist::fromJson(j.is_discarded() ? nlohmann::json() : j);
                        if (i < c.items.size()) { c.items[i].done = on; c.titleId = id; writeAll(hnp::checklistPath(id), c.toJson().dump(1)); }
                    });
                    list->addItem(t);
                }
            }
        }
        frame->setContent(list);
        return frame;
    }
};

class HnpOverlay : public tsl::Overlay {
public:
    void initServices() override {
        pmdmntInitialize();
        pminfoInitialize();
    }
    void exitServices() override {
        pminfoExit();
        pmdmntExit();
    }
    std::unique_ptr<tsl::Gui> loadInitialGui() override { return initially<HnpGui>(); }
};

} // namespace

int main(int argc, char** argv) { return tsl::loop<HnpOverlay>(argc, argv); }
