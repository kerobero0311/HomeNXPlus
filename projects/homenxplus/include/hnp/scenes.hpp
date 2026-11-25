// HomeNX+ : scenes (profil sys-clk + filtre Fizeau + luminosite) et garde batterie (AutoRCM).
// Logique pure, testable sur PC ; stocke dans sdmc:/homenxplus/scenes.json.
// L'application reelle (IPC sys-clk / Fizeau, luminosite) est faite cote Switch.
#pragma once

#include <nlohmann/json.hpp>

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace hnp {

using json = nlohmann::json;

struct Scene {
    std::string id;              // "nuit", "batterie", "perf", ou perso
    std::string name;            // nom affiche
    // sys-clk : 0 = ne pas toucher (laisse le profil sys-clk / le jeu decider). MHz.
    int cpuMHz = 0, gpuMHz = 0, memMHz = 0;
    // Fizeau : temperature de couleur en kelvins (0 = ne pas toucher), filtre ecran actif ou non
    int colorTempK = 0;
    bool fizeauOn = false;
    // luminosite 0.0-1.0 ; -1 = ne pas toucher
    double brightness = -1;
    bool builtIn = false;        // les 3 scenes fournies ne se suppriment pas
};

// Frequences officielles Erista (en MHz) acceptees par sys-clk : on refuse le reste.
bool isValidCpu(int mhz);
bool isValidGpu(int mhz);
bool isValidMem(int mhz);

class Scenes {
public:
    Scenes();                                    // contient Nuit, Batterie, Perf max
    const std::vector<Scene>& list() const { return m_list; }
    const Scene* find(const std::string& id) const;
    bool upsert(const Scene& s);                 // ajoute ou remplace (valeurs verifiees) ; false si invalide
    bool remove(const std::string& id);          // refuse pour les scenes fournies
    // Scene automatique par jeu (TitleID -> id de scene)
    void setForGame(const std::string& titleId, const std::string& sceneId);  // "" = aucune
    std::optional<std::string> forGame(const std::string& titleId) const;
    std::string active;                          // scene en cours (affichee par le widget)

    json toJson() const;
    static Scenes fromJson(const json& j);

private:
    std::vector<Scene> m_list;
    std::map<std::string, std::string> m_perGame;
};

// ---------------------------------------------------------------- garde batterie
enum class BatteryAlert { None, Low15, Critical8 };

// Memorise la derniere alerte pour ne pas repeter le message a chaque mesure.
class BatteryGuard {
public:
    // A appeler a chaque mesure ; renvoie l'alerte a afficher MAINTENANT (ou None).
    BatteryAlert update(int percent, bool charging);
    static std::string message(BatteryAlert a);  // texte en francais avec le rappel AutoRCM
private:
    BatteryAlert m_last = BatteryAlert::None;
};

} // namespace hnp
