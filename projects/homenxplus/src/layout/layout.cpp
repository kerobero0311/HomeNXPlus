// HomeNX+ : implementation du modele de rangement.
#include "hnp/layout.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>

namespace hnp {

namespace fs = std::filesystem;
static const std::string kFolderPrefix = "folder:";

const char* platformName(Platform p) {
    switch (p) {
        case Platform::Switch:   return "Switch";
        case Platform::WiiGC:    return "Wii / GameCube";
        case Platform::Retro:    return "Retro";
        case Platform::Homebrew: return "Homebrew";
        case Platform::Web:      return "Web";
        case Platform::System:   return "Systeme";
    }
    return "?";
}

const char* sortName(SortMode s) {
    switch (s) {
        case SortMode::Custom:    return "Mon ordre";
        case SortMode::AZ:        return "A-Z";
        case SortMode::Recent:    return "Recents";
        case SortMode::PlayTime:  return "Temps de jeu";
        case SortMode::Installed: return "Date d'installation";
        case SortMode::Size:      return "Taille";
        case SortMode::Platform:  return "Plateforme";
    }
    return "?";
}

static const char* kSortKeys[] = {"custom", "az", "recent", "playtime", "installed", "size", "platform"};
std::string sortToString(SortMode s) { return kSortKeys[(int)s]; }
SortMode sortFromString(const std::string& s) {
    for (int i = 0; i < 7; ++i) if (s == kSortKeys[i]) return (SortMode)i;
    return SortMode::Custom;
}

const char* smartName(Smart s) {
    switch (s) {
        case Smart::Recent:        return "Recents";
        case Smart::Favorites:     return "Favoris";
        case Smart::NeverLaunched: return "Jamais lances";
        case Smart::MostPlayed:    return "Les plus joues";
        case Smart::Switch:        return "Switch";
        case Smart::WiiGC:         return "Wii / GameCube";
        case Smart::Retro:         return "Retro";
        case Smart::Homebrew:      return "Homebrew";
        case Smart::LocalMulti:    return "Multijoueur local";
    }
    return "?";
}

// ---------------------------------------------------------------- recherche
std::string normalizeForSearch(const std::string& s) {
    std::string out;
    for (std::size_t i = 0; i < s.size();) {
        unsigned char c = (unsigned char)s[i];
        if (c < 0x80) { out.push_back((char)std::tolower(c)); ++i; continue; }
        if (c == 0xC3 && i + 1 < s.size()) {  // Latin-1 accentue (U+00C0..U+00FF)
            unsigned char d = (unsigned char)s[i + 1] | 0x20;  // minuscule
            const char* m = nullptr;
            switch (d) {
                case 0xA0: case 0xA1: case 0xA2: case 0xA3: case 0xA4: case 0xA5: m = "a"; break;
                case 0xA7: m = "c"; break;
                case 0xA8: case 0xA9: case 0xAA: case 0xAB: m = "e"; break;
                case 0xAC: case 0xAD: case 0xAE: case 0xAF: m = "i"; break;
                case 0xB1: m = "n"; break;
                case 0xB2: case 0xB3: case 0xB4: case 0xB5: case 0xB6: m = "o"; break;
                case 0xB9: case 0xBA: case 0xBB: case 0xBC: m = "u"; break;
                case 0xBD: case 0xBF: m = "y"; break;
            }
            if (m) { out += m; i += 2; continue; }
        }
        out.push_back((char)c); ++i;  // autre caractere (japonais...) : garde tel quel
    }
    return out;
}

// ---------------------------------------------------------------- tri / filtre / intelligents
std::vector<std::string> sortIds(std::vector<std::string> ids, SortMode mode, const std::map<std::string, Item>& items) {
    if (mode == SortMode::Custom) return ids;
    auto get = [&](const std::string& id) -> const Item* { auto it = items.find(id); return it == items.end() ? nullptr : &it->second; };
    std::stable_sort(ids.begin(), ids.end(), [&](const std::string& a, const std::string& b) {
        const Item* x = get(a); const Item* y = get(b);
        if (!x || !y) return x != nullptr && y == nullptr;  // inconnus a la fin
        switch (mode) {
            case SortMode::AZ:        return normalizeForSearch(x->title) < normalizeForSearch(y->title);
            case SortMode::Recent:    return x->lastPlayed > y->lastPlayed;
            case SortMode::PlayTime:  return x->playMinutes > y->playMinutes;
            case SortMode::Installed: return x->installed > y->installed;
            case SortMode::Size:      return x->sizeBytes > y->sizeBytes;
            case SortMode::Platform:
                if (x->platform != y->platform) return (int)x->platform < (int)y->platform;
                return normalizeForSearch(x->title) < normalizeForSearch(y->title);
            default: return false;
        }
    });
    return ids;
}

std::vector<std::string> filterIds(const std::vector<std::string>& ids, const std::map<std::string, Item>& items,
                                   const std::string& query, int platform, bool favoritesOnly,
                                   const std::set<std::string>& favorites) {
    const std::string q = normalizeForSearch(query);
    std::vector<std::string> out;
    for (const auto& id : ids) {
        auto it = items.find(id);
        if (it == items.end()) continue;
        if (platform >= 0 && (int)it->second.platform != platform) continue;
        if (favoritesOnly && !favorites.count(id)) continue;
        if (!q.empty() && normalizeForSearch(it->second.title).find(q) == std::string::npos) continue;
        out.push_back(id);
    }
    return out;
}

std::vector<std::string> smartFolder(Smart s, const std::map<std::string, Item>& items,
                                     const std::set<std::string>& favorites, std::size_t limit) {
    std::vector<std::string> ids;
    for (const auto& [id, it] : items) {
        bool keep = false;
        switch (s) {
            case Smart::Recent:        keep = it.lastPlayed > 0; break;
            case Smart::Favorites:     keep = favorites.count(id) > 0; break;
            case Smart::NeverLaunched: keep = it.lastPlayed == 0 && it.launches == 0 && it.platform != Platform::System; break;
            case Smart::MostPlayed:    keep = it.playMinutes > 0; break;
            case Smart::Switch:        keep = it.platform == Platform::Switch; break;
            case Smart::WiiGC:         keep = it.platform == Platform::WiiGC; break;
            case Smart::Retro:         keep = it.platform == Platform::Retro; break;
            case Smart::Homebrew:      keep = it.platform == Platform::Homebrew; break;
            case Smart::LocalMulti:    keep = it.localMultiplayer; break;
        }
        if (keep) ids.push_back(id);
    }
    SortMode m = s == Smart::Recent ? SortMode::Recent : s == Smart::MostPlayed ? SortMode::PlayTime : SortMode::AZ;
    ids = sortIds(ids, m, items);
    if ((s == Smart::Recent || s == Smart::MostPlayed) && ids.size() > limit) ids.resize(limit);
    return ids;
}

// ---------------------------------------------------------------- Layout
void Layout::snapshot() {
    m_undo.push_back(toJson());
    if (m_undo.size() > 20) m_undo.pop_front();
}

bool Layout::undo() {
    if (m_undo.empty()) return false;
    json j = m_undo.back(); m_undo.pop_back();
    auto saved = std::move(m_undo);
    *this = fromJson(j);
    m_undo = std::move(saved);
    return true;
}

std::string Layout::folderOf(const std::string& itemId) const {
    for (const auto& [id, f] : folders)
        if (std::find(f.items.begin(), f.items.end(), itemId) != f.items.end()) return id;
    return "";
}

static void eraseValue(std::vector<std::string>& v, const std::string& x) { v.erase(std::remove(v.begin(), v.end(), x), v.end()); }

std::string Layout::createFolder(const std::string& name, const std::string& parent) {
    if (name.empty()) return "";
    if (!parent.empty()) {
        auto p = folders.find(parent);
        if (p == folders.end() || !p->second.parent.empty()) return "";  // sous-dossiers sur 1 niveau seulement
    }
    snapshot();
    std::string id;
    do { id = "f" + std::to_string(m_nextId++); } while (folders.count(id));
    Folder f; f.id = id; f.name = name; f.parent = parent;
    folders[id] = f;
    if (parent.empty()) home.push_back(kFolderPrefix + id);
    else folders[parent].items.push_back(kFolderPrefix + id);
    return id;
}

bool Layout::renameFolder(const std::string& id, const std::string& name) {
    auto it = folders.find(id);
    if (it == folders.end() || name.empty()) return false;
    snapshot(); it->second.name = name; return true;
}

bool Layout::deleteFolder(const std::string& id) {
    auto it = folders.find(id);
    if (it == folders.end()) return false;
    snapshot();
    Folder f = it->second;
    std::vector<std::string>& dest = f.parent.empty() ? home : folders[f.parent].items;
    auto pos = std::find(dest.begin(), dest.end(), kFolderPrefix + id);
    std::size_t at = pos == dest.end() ? dest.size() : (std::size_t)(pos - dest.begin());
    if (pos != dest.end()) dest.erase(pos);
    // le contenu (elements et sous-dossiers) prend la place du dossier supprime
    dest.insert(dest.begin() + (std::ptrdiff_t)at, f.items.begin(), f.items.end());
    for (auto& [sid, sf] : folders) if (sf.parent == id) sf.parent = f.parent;
    folders.erase(id);
    return true;
}

bool Layout::setFolderLook(const std::string& id, const std::string& color, const std::string& icon,
                           const std::string& background, const std::string& style) {
    auto it = folders.find(id);
    if (it == folders.end()) return false;
    snapshot();
    it->second.color = color; it->second.icon = icon; it->second.background = background; it->second.style = style;
    return true;
}

bool Layout::setSort(const std::string& folderId, SortMode s) {
    if (!folderId.empty() && !folders.count(folderId)) return false;
    snapshot();
    if (folderId.empty()) homeSort = s; else folders[folderId].sort = s;
    return true;
}

bool Layout::moveItems(const std::vector<std::string>& ids, const std::string& folderId, int position) {
    if (!folderId.empty() && !folders.count(folderId)) return false;
    for (const auto& id : ids) if (id.rfind(kFolderPrefix, 0) == 0) return false;  // un dossier ne se deplace pas ainsi
    snapshot();
    for (const auto& id : ids) { eraseValue(home, id); for (auto& [fid, f] : folders) eraseValue(f.items, id); }
    std::vector<std::string>& dest = folderId.empty() ? home : folders[folderId].items;
    std::size_t at = position < 0 || (std::size_t)position > dest.size() ? dest.size() : (std::size_t)position;
    dest.insert(dest.begin() + (std::ptrdiff_t)at, ids.begin(), ids.end());
    return true;
}

bool Layout::setFavorite(const std::string& id, bool on) {
    snapshot(); if (on) favorites.insert(id); else favorites.erase(id); return true;
}
bool Layout::setPinned(const std::string& id, bool on) {
    snapshot(); eraseValue(pinned, id); if (on) pinned.push_back(id); return true;
}
bool Layout::setHidden(const std::string& id, bool on) {
    snapshot(); if (on) hidden.insert(id); else hidden.erase(id); return true;
}

void Layout::sync(const std::vector<Item>& all) {
    std::set<std::string> exists, placed;
    for (const auto& it : all) exists.insert(it.id);
    auto clean = [&](std::vector<std::string>& v) {
        v.erase(std::remove_if(v.begin(), v.end(), [&](const std::string& x) {
            if (x.rfind(kFolderPrefix, 0) == 0) return !folders.count(x.substr(kFolderPrefix.size()));
            return !exists.count(x);
        }), v.end());
        for (auto& x : v) placed.insert(x);
    };
    clean(home);
    for (auto& [id, f] : folders) clean(f.items);
    eraseValue(pinned, ""); pinned.erase(std::remove_if(pinned.begin(), pinned.end(), [&](const std::string& x) { return !exists.count(x); }), pinned.end());
    for (const auto& it : all) if (!placed.count(it.id)) home.push_back(it.id);  // nouveaux jeux a la fin de l'accueil
}

std::vector<std::string> Layout::view(const std::string& folderId, const std::map<std::string, Item>& items) const {
    const std::vector<std::string>* src = &home;
    SortMode mode = homeSort;
    if (!folderId.empty()) {
        auto it = folders.find(folderId);
        if (it == folders.end()) return {};
        src = &it->second.items; mode = it->second.sort;
    }
    std::vector<std::string> subfolders, games;
    for (const auto& x : *src) {
        if (hidden.count(x)) continue;
        (x.rfind(kFolderPrefix, 0) == 0 ? subfolders : games).push_back(x);
    }
    if (mode != SortMode::Custom) {
        games = sortIds(games, mode, items);
        std::vector<std::string> out = subfolders;  // dossiers d'abord quand on trie
        out.insert(out.end(), games.begin(), games.end());
        games = out;
    } else {
        games.clear();
        for (const auto& x : *src) if (!hidden.count(x)) games.push_back(x);
    }
    if (folderId.empty() && !pinned.empty()) {  // epingles en tete de l'accueil
        std::vector<std::string> out;
        for (const auto& p : pinned) if (!hidden.count(p) && items.count(p)) out.push_back(p);
        for (const auto& x : games) if (std::find(pinned.begin(), pinned.end(), x) == pinned.end()) out.push_back(x);
        return out;
    }
    return games;
}

// ---------------------------------------------------------------- JSON
json Layout::toJson() const {
    json j;
    j["version"] = 1;
    j["home"] = home;
    j["homeSort"] = sortToString(homeSort);
    j["homeStyle"] = homeStyle;
    j["favorites"] = favorites;
    j["pinned"] = pinned;
    j["hidden"] = hidden;
    j["nextId"] = m_nextId;
    j["folders"] = json::array();
    for (const auto& [id, f] : folders)
        j["folders"].push_back({{"id", f.id}, {"name", f.name}, {"parent", f.parent}, {"color", f.color}, {"icon", f.icon},
                                {"background", f.background}, {"style", f.style}, {"sort", sortToString(f.sort)}, {"items", f.items}});
    return j;
}

Layout Layout::fromJson(const json& j) {
    Layout l;
    if (!j.is_object()) return l;
    auto strs = [](const json& a) { std::vector<std::string> v; if (a.is_array()) for (auto& x : a) if (x.is_string()) v.push_back(x); return v; };
    l.home = strs(j.value("home", json::array()));
    l.homeSort = sortFromString(j.value("homeSort", "custom"));
    l.homeStyle = j.value("homeStyle", "cards");
    for (auto& s : strs(j.value("favorites", json::array()))) l.favorites.insert(s);
    l.pinned = strs(j.value("pinned", json::array()));
    for (auto& s : strs(j.value("hidden", json::array()))) l.hidden.insert(s);
    l.m_nextId = j.value("nextId", 1);
    for (auto& f : j.value("folders", json::array())) {
        if (!f.is_object() || !f.contains("id")) continue;
        Folder d; d.id = f.value("id", ""); d.name = f.value("name", ""); d.parent = f.value("parent", "");
        d.color = f.value("color", "#4C8DFF"); d.icon = f.value("icon", ""); d.background = f.value("background", "");
        d.style = f.value("style", ""); d.sort = sortFromString(f.value("sort", "custom")); d.items = strs(f.value("items", json::array()));
        if (!d.id.empty()) l.folders[d.id] = d;
    }
    return l;
}

Layout Layout::importCardsNX(const json& s) {
    Layout l;
    if (!s.is_object()) return l;
    if (s.contains("orderGames") && s["orderGames"].is_array())
        for (auto& x : s["orderGames"]) if (x.is_string()) l.home.push_back(x);
    if (s.contains("hiddenCards") && s["hiddenCards"].is_object())
        for (auto& [k, v] : s["hiddenCards"].items()) if (v.is_boolean() ? v.get<bool>() : true) l.hidden.insert(k);
    return l;
}

// ---------------------------------------------------------------- fichiers
bool saveWithBackups(const std::string& path, const std::string& content, int keep) {
    std::error_code ec;
    const std::string tmp = path + ".tmp";
    {
        std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
        if (!out) return false;
        out << content;
        if (!out.good()) return false;
    }
    if (fs::exists(path, ec)) {  // rotation : path.1 (recent) ... path.keep (ancien)
        fs::remove(path + "." + std::to_string(keep), ec);
        for (int i = keep - 1; i >= 1; --i)
            if (fs::exists(path + "." + std::to_string(i), ec))
                fs::rename(path + "." + std::to_string(i), path + "." + std::to_string(i + 1), ec);
        fs::rename(path, path + ".1", ec);
    }
    fs::rename(tmp, path, ec);
    return !ec && fs::exists(path, ec);
}

} // namespace hnp
