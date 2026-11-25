// HomeNX+ : notes et checklist de completion par jeu. Logique pure, testable sur PC.
// Fichiers : sdmc:/homenxplus/notes/<TitleID>.json   et   sdmc:/homenxplus/checklists/<TitleID>.json
#pragma once

#include <nlohmann/json.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace hnp {

using json = nlohmann::json;

struct GameNotes {
    std::string titleId;
    std::string text;                 // notes libres
    std::int64_t updated = 0;         // horodatage Unix
    json toJson() const;
    static GameNotes fromJson(const json& j);
};

struct CheckItem { std::string label; bool done = false; };

struct Checklist {
    std::string titleId;
    std::string source = "manuel";    // "manuel" ou "claude" (validee par l'utilisateur)
    std::vector<CheckItem> items;
    int doneCount() const;
    int percent() const;              // 0-100
    bool toggle(std::size_t i);
    json toJson() const;
    static Checklist fromJson(const json& j);
    // Texte de Claude (une tache par ligne, avec ou sans "- ", "1.", "[ ]") -> elements
    static Checklist fromText(const std::string& titleId, const std::string& text);
};

bool isTitleId(const std::string& s);           // 16 chiffres hexadecimaux
std::string notesPath(const std::string& titleId);
std::string checklistPath(const std::string& titleId);

} // namespace hnp
