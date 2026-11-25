// HomeNX+ : rangement assisté par Claude (logique pure, testée sur PC).
// Claude propose des catégories pour les jeux installés ; l'utilisateur accepte ou refuse,
// et chaque catégorie devient une page perso de l'apparence Cartes.
#pragma once

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace hnp {

using Categories = std::vector<std::pair<std::string, std::vector<std::string>>>;

// La question posée à Claude, avec la liste exacte des titres (150 au plus).
std::string categoriesPrompt(const std::vector<std::string>& titles);

// Lit la réponse de Claude ({"Catégorie": ["titre", ...], ...}, même entourée de texte ou de ```).
// Ne garde que les titres de `titles` (comparés sans casse ni ponctuation), chacun une seule fois,
// renvoyés tels qu'ils sont écrits dans `titles` ; retire les catégories vides ; `maxCategories` au plus.
Categories parseCategories(const std::string& reply, const std::vector<std::string>& titles, std::size_t maxCategories = 8);

} // namespace hnp
