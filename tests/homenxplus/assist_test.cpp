// Tests du rangement assisté (hnp/assist).
#include "hnp/assist.hpp"

#include <cassert>
#include <cstdio>

int main() {
    const std::vector<std::string> titles = {"Super Mario Odyssey", "Mario Kart 8 Deluxe", "The Legend of Zelda: Tears of the Kingdom",
                                             "Xenoblade Chronicles", "Pokémon Écarlate"};

    // Le prompt contient chaque titre
    const std::string p = hnp::categoriesPrompt(titles);
    for (const auto& t : titles) assert(p.find(t) != std::string::npos);

    // Réponse entourée de ``` et de texte, titres avec casse ou ponctuation différentes, un titre inventé
    const std::string reply = "Voici :\n```json\n{\"Aventure\": [\"super mario odyssey\", \"The Legend of Zelda - Tears of the Kingdom\"],"
                              " \"Course\": [\"Mario Kart 8 Deluxe\", \"F-Zero 99\"],"
                              " \"RPG\": [\"Xenoblade Chronicles\", \"Pokémon Écarlate\", \"Super Mario Odyssey\"],"
                              " \"Vide\": [\"Inconnu\"]}\n```";
    const hnp::Categories c = hnp::parseCategories(reply, titles);
    assert(c.size() == 3);                                  // « Vide » n'a aucun titre connu
    assert(c[0].first == "Aventure" && c[0].second.size() == 2);
    assert(c[0].second[0] == "Super Mario Odyssey");        // rendu tel qu'écrit dans la liste
    assert(c[0].second[1] == "The Legend of Zelda: Tears of the Kingdom");
    assert(c[1].first == "Course" && c[1].second.size() == 1);   // F-Zero 99 n'est pas installé
    assert(c[2].first == "RPG" && c[2].second.size() == 2);      // Odyssey déjà rangé : pas en double

    // Limite du nombre de catégories, réponses invalides
    assert(hnp::parseCategories(reply, titles, 1).size() == 1);
    assert(hnp::parseCategories("pas de JSON", titles).empty());
    assert(hnp::parseCategories("{\"A\": \"pas une liste\"}", titles).empty());
    assert(hnp::parseCategories("[1,2]", titles).empty());

    std::puts("hnp_assist OK");
    return 0;
}
