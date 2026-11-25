// HomeNX+ : demandes ponctuelles a Claude depuis le menu (checklist d'un jeu...). Switch uniquement.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace hnp {

// Envoie une question a Claude (cle et plafond de config/homenxplus), attend toute la reponse.
// Renvoie true + le texte dans `out`, ou false + un message d'erreur en francais dans `out`.
// Le cout est ajoute au registre du mois (homenxplus/claude/usage-AAAA-MM.csv).
bool askClaudeOnce(const std::string& prompt, std::string& out, int maxTokens = 2000,
                   const std::vector<std::uint8_t>* jpeg = nullptr);

// Derniere capture d'ecran de l'Album (lecture seule). `when` = date lisible.
bool lastScreenshot(std::vector<std::uint8_t>& jpeg, std::string& when);
std::string translatePrompt();

// Prompt de checklist de completion pour un jeu (texte en francais, une tache par ligne).
std::string checklistPrompt(const std::string& gameName);

} // namespace hnp
