// HomeNX+ : mise à jour depuis GitHub et captures d'un jeu (Switch uniquement, sauf compareVersions).
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace hnp {

// Compare deux versions « 0.9.3 » / « v0.9.4 » : <0 si a<b, 0 si égales, >0 si a>b.
int compareVersions(const std::string& a, const std::string& b);

struct UpdateInfo {
    bool ok = false;            // la vérification a abouti
    std::string version;        // dernière version publiée (sans le « v »)
    std::string url;            // adresse du .nro à télécharger
    std::string notes;          // texte de la version (début)
    std::string error;          // message en français si !ok
};

// Dernière version de HomeNX+ publiée sur github.com/kerobero0311/HomeNXPlus.
UpdateInfo checkUpdate();
// Télécharge `url` dans `dest` (fichier .part puis renommage, NRO vérifié). Message en français.
bool downloadNro(const std::string& url, const std::string& dest, std::string& message);

// Captures de l'Album d'un jeu, les plus récentes d'abord : « 25/11/2026 14:02 · photo ».
std::vector<std::string> capturesOfGame(std::uint64_t titleId, int max = 30);

} // namespace hnp
