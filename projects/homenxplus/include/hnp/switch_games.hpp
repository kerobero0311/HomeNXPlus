// HomeNX+ : jeux installes et historique de jeu (Switch uniquement, lecture seule).
#pragma once

#include "hnp/layout.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace hnp {

std::string gameName(std::uint64_t titleId);              // nom dans la langue de la console
std::vector<Item> installedGames(std::size_t max = 400);  // jeux + temps de jeu + derniere partie
std::uint64_t lastPlayedGame();                           // dernier jeu de l'utilisateur courant (0 = aucun)

} // namespace hnp
