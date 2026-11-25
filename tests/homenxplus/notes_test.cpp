// Tests PC des notes et checklists par jeu.
#include "hnp/notes.hpp"

#include <cassert>
#include <cstdio>

using namespace hnp;

int main() {
    assert(isTitleId("0100152000022000") && !isTitleId("0100") && !isTitleId("010015200002200Z"));
    assert(notesPath("0100152000022000") == "sdmc:/homenxplus/notes/0100152000022000.json");

    GameNotes n; n.titleId = "0100152000022000"; n.text = "Raccourci : 3e virage, à gauche"; n.updated = 1795500000;
    GameNotes r = GameNotes::fromJson(json::parse(n.toJson().dump()));
    assert(r.text == n.text && r.updated == n.updated && GameNotes::fromJson(json(3)).text.empty());

    const char* texte =
        "Voici ta checklist :\n"
        "- Finir les 8 coupes 150cc\n"
        "* Débloquer Bowser Squelette\n"
        "1. Gagner une coupe en 200cc\n"
        "2) Toutes les étoiles en contre-la-montre\n"
        "• Obtenir le planeur doré\n"
        "[x] Coupe Champignon\n"
        "[ ] Coupe Fleur\n"
        "\n   \n";
    Checklist c = Checklist::fromText("0100152000022000", texte);
    assert(c.source == "claude" && c.items.size() == 7);
    assert(c.items[0].label == "Finir les 8 coupes 150cc");
    assert(c.items[3].label == "Toutes les étoiles en contre-la-montre");
    assert(c.items[4].label == "Obtenir le planeur doré");
    assert(c.items[5].done && c.items[5].label == "Coupe Champignon" && !c.items[6].done);
    assert(c.doneCount() == 1 && c.percent() == 14);
    assert(c.toggle(6) && c.doneCount() == 2 && !c.toggle(99));
    Checklist d = Checklist::fromJson(json::parse(c.toJson().dump()));
    assert(d.items.size() == 7 && d.items[6].done && d.percent() == 28);
    assert(Checklist().percent() == 0);
    std::puts("notes et checklists : tous les tests passent");
    return 0;
}
