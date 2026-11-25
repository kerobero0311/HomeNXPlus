// HomeNX+ : mode sans echec. Chaque demarrage du menu incremente un compteur ; il est remis a 0
// apres 30 s de fonctionnement ou a une sortie propre. Apres 3 demarrages rates de suite, le menu
// demarre en mode allege (sans effets de verre, sans musique, sans images SteamGridDB).
#pragma once

#include <string>

namespace hnp {

extern bool g_safeMode;          // vrai pendant ce demarrage si le mode sans echec est actif
extern int g_failedStarts;       // demarrages rates avant celui-ci

constexpr int kSafeModeThreshold = 3;

// Lit le compteur, l'incremente sur la carte et renvoie la valeur d'avant (= demarrages rates).
int beginStart(const std::string& counterPath);
// Le menu tourne correctement : compteur remis a 0.
void markHealthy(const std::string& counterPath);
inline bool isSafe(int failedStarts) { return failedStarts >= kSafeModeThreshold; }

} // namespace hnp
