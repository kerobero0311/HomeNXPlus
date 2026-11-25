// HomeNX+ : notes et checklists par jeu.
#include "hnp/notes.hpp"

#include <cctype>
#include <sstream>

namespace hnp {

bool isTitleId(const std::string& s) {
    if (s.size() != 16) return false;
    for (char c : s) if (!std::isxdigit((unsigned char)c)) return false;
    return true;
}

std::string notesPath(const std::string& t) { return "sdmc:/homenxplus/notes/" + t + ".json"; }
std::string checklistPath(const std::string& t) { return "sdmc:/homenxplus/checklists/" + t + ".json"; }

json GameNotes::toJson() const { return {{"titleId", titleId}, {"text", text}, {"updated", updated}}; }
GameNotes GameNotes::fromJson(const json& j) {
    GameNotes n;
    if (!j.is_object()) return n;
    n.titleId = j.value("titleId", ""); n.text = j.value("text", ""); n.updated = j.value("updated", (std::int64_t)0);
    return n;
}

int Checklist::doneCount() const { int n = 0; for (const auto& i : items) n += i.done; return n; }
int Checklist::percent() const { return items.empty() ? 0 : doneCount() * 100 / (int)items.size(); }
bool Checklist::toggle(std::size_t i) { if (i >= items.size()) return false; items[i].done = !items[i].done; return true; }

json Checklist::toJson() const {
    json a = json::array();
    for (const auto& i : items) a.push_back({{"label", i.label}, {"done", i.done}});
    return {{"titleId", titleId}, {"source", source}, {"items", a}};
}

Checklist Checklist::fromJson(const json& j) {
    Checklist c;
    if (!j.is_object()) return c;
    c.titleId = j.value("titleId", ""); c.source = j.value("source", "manuel");
    for (const auto& x : j.value("items", json::array()))
        if (x.is_object() && x.contains("label")) c.items.push_back({x.value("label", ""), x.value("done", false)});
    return c;
}

Checklist Checklist::fromText(const std::string& titleId, const std::string& text) {
    Checklist c; c.titleId = titleId; c.source = "claude";
    std::istringstream in(text);
    std::string line;
    while (std::getline(in, line)) {
        std::size_t a = 0;
        while (a < line.size() && std::isspace((unsigned char)line[a])) ++a;
        bool done = false;
        // puces et numeros : "- ", "* ", "• ", "1. ", "1) ", "[ ] ", "[x] "
        if (line.compare(a, 3, "\xE2\x80\xA2") == 0) a += 3;
        else if (a < line.size() && (line[a] == '-' || line[a] == '*')) ++a;
        else { std::size_t d = a; while (d < line.size() && std::isdigit((unsigned char)line[d])) ++d;
               if (d > a && d < line.size() && (line[d] == '.' || line[d] == ')')) a = d + 1; }
        while (a < line.size() && std::isspace((unsigned char)line[a])) ++a;
        if (line.compare(a, 3, "[ ]") == 0) a += 3;
        else if (line.compare(a, 3, "[x]") == 0 || line.compare(a, 3, "[X]") == 0) { a += 3; done = true; }
        while (a < line.size() && std::isspace((unsigned char)line[a])) ++a;
        std::string label = line.substr(a);
        while (!label.empty() && std::isspace((unsigned char)label.back())) label.pop_back();
        if (label.empty() || label.back() == ':') continue;  // lignes vides et titres de section
        if (label.size() > 200) label.resize(200);
        c.items.push_back({label, done});
        if (c.items.size() >= 100) break;
    }
    return c;
}

} // namespace hnp
