// HomeNX+ : rangement assisté par Claude (voir hnp/assist.hpp).
#include "hnp/assist.hpp"

#include <nlohmann/json.hpp>

#include <cctype>
#include <map>
#include <set>

namespace hnp {
namespace {

// Clé de comparaison : minuscules, lettres et chiffres seulement (les octets UTF-8 restent tels quels).
std::string key(const std::string& s) {
    std::string out;
    for (unsigned char c : s) {
        if (c >= 0x80) out += static_cast<char>(c);
        else if (std::isalnum(c)) out += static_cast<char>(std::tolower(c));
    }
    return out;
}

} // namespace

std::string categoriesPrompt(const std::vector<std::string>& titles) {
    std::string list;
    std::size_t n = 0;
    for (const std::string& t : titles) {
        if (++n > 150) break;
        list += "- " + t + "\n";
    }
    return "Voici les jeux installés sur ma Nintendo Switch. Range-les en 3 à 6 catégories courtes, en français "
           "(par exemple : Aventure, Plateforme, Course, Fête, RPG, Sport, Réflexion). Chaque jeu dans une seule catégorie. "
           "Réponds UNIQUEMENT par un objet JSON de la forme {\"Catégorie\": [\"titre exact\", ...]}, "
           "en recopiant les titres exactement comme dans la liste, sans rien d'autre.\n\n" + list;
}

Categories parseCategories(const std::string& reply, const std::vector<std::string>& titles, std::size_t maxCategories) {
    Categories out;
    const std::size_t a = reply.find('{'), b = reply.rfind('}');
    if (a == std::string::npos || b == std::string::npos || b <= a) return out;
    const nlohmann::json j = nlohmann::json::parse(reply.substr(a, b - a + 1), nullptr, false);
    if (j.is_discarded() || !j.is_object()) return out;

    std::map<std::string, std::string> known;  // clé -> titre exact
    for (const std::string& t : titles) known.emplace(key(t), t);
    std::set<std::string> used;
    for (auto it = j.begin(); it != j.end() && out.size() < maxCategories; ++it) {
        std::string name = it.key();
        while (!name.empty() && std::isspace(static_cast<unsigned char>(name.back()))) name.pop_back();
        if (name.empty() || name.size() > 32 || !it.value().is_array()) continue;
        std::vector<std::string> games;
        for (const auto& v : it.value()) {
            if (!v.is_string()) continue;
            const auto found = known.find(key(v.get<std::string>()));
            if (found == known.end() || !used.insert(found->second).second) continue;
            games.push_back(found->second);
        }
        if (!games.empty()) out.emplace_back(name, std::move(games));
    }
    return out;
}

} // namespace hnp
