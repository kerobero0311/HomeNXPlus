// HomeNX+ : modele de rangement (dossiers, ordre, favoris, epinglage, tris, filtres, dossiers
// intelligents, annulation, sauvegardes). Logique pure, testable sur PC ; stocke dans
// sdmc:/homenxplus/layout.json (+ 5 versions precedentes layout.json.1 ... .5).
#pragma once

#include <nlohmann/json.hpp>

#include <cstdint>
#include <deque>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace hnp {

using json = nlohmann::json;

enum class Platform { Switch, WiiGC, Retro, Homebrew, Web, System };
enum class SortMode { Custom, AZ, Recent, PlayTime, Installed, Size, Platform };

const char* platformName(Platform p);   // nom affiche en francais
const char* sortName(SortMode s);
SortMode sortFromString(const std::string& s);
std::string sortToString(SortMode s);

// Un element affichable (jeu, homebrew, raccourci). Les donnees viennent du systeme au demarrage.
struct Item {
    std::string id;                 // TitleID en hexa (16 chiffres) ou "nro:sdmc:/switch/x.nro"
    std::string title;
    Platform platform = Platform::Switch;
    std::int64_t playMinutes = 0;
    std::int64_t lastPlayed = 0;    // horodatage Unix, 0 = jamais lance
    std::int64_t installed = 0;     // horodatage Unix
    std::int64_t sizeBytes = 0;
    int launches = 0;
    bool localMultiplayer = false;
};

struct Folder {
    std::string id;                 // "f1", "f2"...
    std::string name;
    std::string parent;             // "" = a l'accueil ; sinon dossier parent (1 niveau max)
    std::string color = "#4C8DFF";
    std::string icon;               // chemin d'image facultatif
    std::string background;         // fond propre au dossier, facultatif
    std::string style;              // style d'interface propre au dossier ("" = celui de l'accueil)
    SortMode sort = SortMode::Custom;
    std::vector<std::string> items; // ids d'elements, dans l'ordre perso
};

// Dossiers intelligents (calcules, jamais modifies a la main)
enum class Smart { Recent, Favorites, NeverLaunched, MostPlayed, Switch, WiiGC, Retro, Homebrew, LocalMulti };
const char* smartName(Smart s);

class Layout {
public:
    // --- etat
    std::vector<std::string> home;              // ordre de l'accueil : ids d'elements ou "folder:<id>"
    std::map<std::string, Folder> folders;
    std::set<std::string> favorites;
    std::vector<std::string> pinned;            // epingles en tete, dans l'ordre
    std::set<std::string> hidden;               // elements masques
    SortMode homeSort = SortMode::Custom;
    std::string homeStyle = "cards";

    // --- dossiers (chaque operation modifiante est annulable)
    std::string createFolder(const std::string& name, const std::string& parent = "");  // "" si refuse
    bool renameFolder(const std::string& id, const std::string& name);
    bool deleteFolder(const std::string& id);   // le contenu revient au parent (ou a l'accueil)
    bool setFolderLook(const std::string& id, const std::string& color, const std::string& icon,
                       const std::string& background, const std::string& style);
    bool setSort(const std::string& folderId, SortMode s);  // "" = accueil
    // Deplace des elements vers un dossier ("" = accueil), position -1 = a la fin
    bool moveItems(const std::vector<std::string>& ids, const std::string& folderId, int position = -1);
    bool setFavorite(const std::string& id, bool on);
    bool setPinned(const std::string& id, bool on);
    bool setHidden(const std::string& id, bool on);

    // --- annulation (20 niveaux)
    bool undo();
    std::size_t undoDepth() const { return m_undo.size(); }

    // --- synchronisation avec la liste reelle : ajoute les nouveaux elements a l'accueil,
    //     retire ceux qui n'existent plus (jeu desinstalle).
    void sync(const std::vector<Item>& all);

    // Elements visibles d'un conteneur ("" = accueil), tries selon son mode, epingles en tete a l'accueil.
    std::vector<std::string> view(const std::string& folderId, const std::map<std::string, Item>& items) const;
    std::string folderOf(const std::string& itemId) const;  // "" = accueil

    // --- JSON
    json toJson() const;
    static Layout fromJson(const json& j);

    // --- import du rangement CardsNX (config/CardsNX/settings.json) : ordre et cartes masquees
    static Layout importCardsNX(const json& cardsSettings);

private:
    void snapshot();
    std::deque<json> m_undo;
    int m_nextId = 1;
};

// --- tri, filtre, recherche, dossiers intelligents
std::vector<std::string> sortIds(std::vector<std::string> ids, SortMode mode, const std::map<std::string, Item>& items);
std::string normalizeForSearch(const std::string& utf8);  // minuscules, sans accents
std::vector<std::string> filterIds(const std::vector<std::string>& ids, const std::map<std::string, Item>& items,
                                   const std::string& query, int platform /* -1 = toutes */, bool favoritesOnly,
                                   const std::set<std::string>& favorites);
std::vector<std::string> smartFolder(Smart s, const std::map<std::string, Item>& items,
                                     const std::set<std::string>& favorites, std::size_t limit = 30);

// --- sauvegarde sure : ecrit path.tmp puis renomme, et garde 5 versions (path.1 = la plus recente)
bool saveWithBackups(const std::string& path, const std::string& content, int keep = 5);

} // namespace hnp
