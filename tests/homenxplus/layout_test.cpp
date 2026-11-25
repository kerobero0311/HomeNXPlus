// Tests PC du modele de rangement de HomeNX+.
#include "hnp/layout.hpp"

#include <cassert>
#include <cstdio>
#include <filesystem>
#include <fstream>

using namespace hnp;

static std::map<std::string, Item> catalogue() {
    std::map<std::string, Item> m;
    auto add = [&](std::string id, std::string t, Platform p, long long min, long long last, long long inst, long long size, bool multi = false) {
        Item i; i.id = id; i.title = t; i.platform = p; i.playMinutes = min; i.lastPlayed = last; i.installed = inst; i.sizeBytes = size; i.localMultiplayer = multi;
        i.launches = last ? 1 : 0; m[id] = i;
    };
    add("0100152000022000", "Mario Kart 8 Deluxe", Platform::Switch, 900, 300, 10, 7000, true);
    add("01006A800016E000", "Super Smash Bros. Ultimate", Platform::Switch, 400, 200, 20, 15000, true);
    add("0100F2C0115B6000", "Zelda : Tears of the Kingdom", Platform::Switch, 0, 0, 30, 16000);
    add("tico:mkwii", "Mario Kart Wii", Platform::WiiGC, 37, 100, 40, 2700, true);
    add("tico:smw", "Super Mario World", Platform::Retro, 5, 50, 50, 1);
    add("nro:sdmc:/switch/ClaudeNX/ClaudeNX.nro", "ClaudeNX", Platform::Homebrew, 0, 0, 60, 1);
    add("0100000000000001", "Évoland", Platform::Switch, 3, 10, 5, 500);
    return m;
}

static std::vector<Item> liste(const std::map<std::string, Item>& m) { std::vector<Item> v; for (auto& [k, i] : m) v.push_back(i); return v; }

static void test_import_et_sync() {
    json cards = {{"orderGames", {"01006A800016E000", "0100152000022000", "0100DEADBEEF0000"}}, {"hiddenCards", {{"0100F2C0115B6000", true}}}};
    Layout l = Layout::importCardsNX(cards);
    assert(l.home.size() == 3 && l.home[0] == "01006A800016E000" && l.hidden.count("0100F2C0115B6000"));
    auto cat = catalogue();
    l.sync(liste(cat));
    assert(std::find(l.home.begin(), l.home.end(), "0100DEADBEEF0000") == l.home.end());  // desinstalle : retire
    assert(l.home[0] == "01006A800016E000" && l.home[1] == "0100152000022000");          // ordre garde
    assert(l.home.size() == cat.size());                                                   // nouveaux ajoutes
    auto v = l.view("", cat);
    assert(std::find(v.begin(), v.end(), "0100F2C0115B6000") == v.end());                 // masque
}

static void test_dossiers_et_annulation() {
    auto cat = catalogue(); Layout l; l.sync(liste(cat));
    std::string course = l.createFolder("Course");
    assert(!course.empty() && l.home.back() == "folder:" + course);
    std::string sous = l.createFolder("Multi", course);
    assert(!sous.empty());
    assert(l.createFolder("Trop profond", sous).empty());  // 1 seul niveau de sous-dossier
    assert(l.moveItems({"0100152000022000", "tico:mkwii"}, course));
    assert(l.folderOf("tico:mkwii") == course && std::find(l.home.begin(), l.home.end(), "tico:mkwii") == l.home.end());
    assert(l.moveItems({"01006A800016E000"}, sous));
    assert(l.renameFolder(course, "Courses"));
    // supprimer : le contenu revient a l'accueil, a la place du dossier
    std::size_t avant = l.home.size();
    assert(l.deleteFolder(course));
    assert(!l.folders.count(course) && l.folders[sous].parent.empty());
    assert(l.home.size() == avant - 1 + 3);  // MK8, MKWii, sous-dossier Multi
    assert(l.folderOf("01006A800016E000") == sous);
    // annulation
    assert(l.undo());
    assert(l.folders.count(course) && l.folders[course].name == "Courses");
    assert(l.undo() && l.folders[course].name == "Course");
    std::size_t profondeur = l.undoDepth();
    for (int i = 0; i < 30; ++i) l.setFavorite("tico:smw", i % 2 == 0);
    assert(l.undoDepth() == 20 && profondeur < 20);  // 20 niveaux maximum
}

static void test_tris_filtres_intelligents() {
    auto cat = catalogue(); Layout l; l.sync(liste(cat));
    std::vector<std::string> ids; for (auto& [k, i] : cat) ids.push_back(k);
    auto az = sortIds(ids, SortMode::AZ, cat);
    assert(cat[az[0]].title == "ClaudeNX" && cat[az[1]].title == "Évoland");  // accents ignores
    assert(sortIds(ids, SortMode::Recent, cat)[0] == "0100152000022000");
    assert(sortIds(ids, SortMode::Size, cat)[0] == "0100F2C0115B6000");
    assert(filterIds(ids, cat, "mario", -1, false, {}).size() == 3);
    assert(filterIds(ids, cat, "EVO", -1, false, {}).size() == 1);
    assert(filterIds(ids, cat, "mario", (int)Platform::WiiGC, false, {}).size() == 1);
    l.setFavorite("tico:smw", true);
    assert(filterIds(ids, cat, "", -1, true, l.favorites) == std::vector<std::string>{"tico:smw"});
    assert(smartFolder(Smart::NeverLaunched, cat, {}).size() == 2);   // Zelda et ClaudeNX
    assert(smartFolder(Smart::LocalMulti, cat, {}).size() == 3);
    assert(smartFolder(Smart::MostPlayed, cat, {}, 2).size() == 2);
    assert(smartFolder(Smart::Recent, cat, {})[0] == "0100152000022000");
    // tri par dossier + epinglage
    assert(l.setSort("", SortMode::AZ));
    l.setPinned("tico:smw", true);
    auto v = l.view("", cat);
    assert(v[0] == "tico:smw" && cat[v[1]].title == "ClaudeNX");
}

static void test_json_et_sauvegardes() {
    auto cat = catalogue(); Layout l; l.sync(liste(cat));
    std::string f = l.createFolder("Rétro"); l.moveItems({"tico:smw"}, f); l.setFavorite("tico:smw", true);
    l.setFolderLook(f, "#FF0000", "", "", "list");
    Layout r = Layout::fromJson(json::parse(l.toJson().dump()));
    assert(r.folders[f].name == "Rétro" && r.folders[f].style == "list" && r.favorites.count("tico:smw") && r.home == l.home);
    assert(Layout::fromJson(json("n'importe quoi")).home.empty());  // fichier abime : rangement vide, pas de plantage
    namespace fs = std::filesystem;
    fs::path dir = fs::temp_directory_path() / "hnp_layout_test"; fs::remove_all(dir); fs::create_directories(dir);
    std::string p = (dir / "layout.json").string();
    for (int i = 1; i <= 8; ++i) assert(saveWithBackups(p, "v" + std::to_string(i)));
    auto lire = [](const std::string& x) { std::ifstream in(x); std::string s; in >> s; return s; };
    assert(lire(p) == "v8" && lire(p + ".1") == "v7" && lire(p + ".5") == "v3" && !fs::exists(p + ".6"));
    assert(!fs::exists(p + ".tmp"));
    fs::remove_all(dir);
}

int main() {
    test_import_et_sync();
    test_dossiers_et_annulation();
    test_tris_filtres_intelligents();
    test_json_et_sauvegardes();
    std::puts("rangement : tous les tests passent");
    return 0;
}
