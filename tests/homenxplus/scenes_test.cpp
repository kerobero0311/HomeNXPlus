// Tests PC des scenes et de la garde batterie.
#include "hnp/scenes.hpp"

#include <cassert>
#include <cstdio>

using namespace hnp;

static void test_scenes() {
    Scenes s;
    assert(s.list().size() == 3 && s.find("nuit") && s.find("perf")->cpuMHz == 1785);
    Scene mk; mk.id = "mk8"; mk.name = "Mario Kart"; mk.cpuMHz = 1224; mk.gpuMHz = 768; mk.brightness = 0.6;
    assert(s.upsert(mk) && s.list().size() == 4);
    Scene bad = mk; bad.cpuMHz = 2000;                        // frequence non officielle : refusee
    assert(!s.upsert(bad));
    bad = mk; bad.brightness = 1.5; assert(!s.upsert(bad));
    bad = mk; bad.colorTempK = 500; assert(!s.upsert(bad));
    assert(!s.remove("nuit"));                                // scene fournie : protegee
    s.setForGame("0100152000022000", "mk8");
    assert(s.forGame("0100152000022000") == std::optional<std::string>("mk8"));
    s.setForGame("0100AAAA00000000", "inconnue");             // scene inexistante : ignore
    assert(!s.forGame("0100AAAA00000000"));
    s.active = "mk8";
    Scenes r = Scenes::fromJson(json::parse(s.toJson().dump()));
    assert(r.list().size() == 4 && r.find("mk8")->gpuMHz == 768 && r.active == "mk8" && r.forGame("0100152000022000"));
    assert(r.remove("mk8") && !r.forGame("0100152000022000") && r.active.empty());
    // fichier abime ou modifie a la main : les scenes fournies restent, les mauvaises valeurs sont ignorees
    json j = {{"scenes", {{{"id", "x"}, {"name", "X"}, {"cpu", 9999}}}}, {"active", "x"}};
    Scenes z = Scenes::fromJson(j);
    assert(z.list().size() == 3 && z.active.empty());
    assert(Scenes::fromJson(json(42)).list().size() == 3);
}

static void test_batterie() {
    BatteryGuard g;
    assert(g.update(50, false) == BatteryAlert::None);
    assert(g.update(15, false) == BatteryAlert::Low15);
    assert(g.update(14, false) == BatteryAlert::None);        // pas de repetition
    assert(g.update(8, false) == BatteryAlert::Critical8);
    assert(g.update(7, false) == BatteryAlert::None);
    assert(g.update(7, true) == BatteryAlert::None);          // branche : rearme
    assert(g.update(12, false) == BatteryAlert::Low15);       // debranche a 12 % : nouvelle alerte
    assert(g.update(17, false) == BatteryAlert::None);
    assert(g.update(15, false) == BatteryAlert::None);        // hysteresis : pas de nouvelle alerte avant 20 %
    assert(g.update(25, false) == BatteryAlert::None);
    assert(g.update(15, false) == BatteryAlert::Low15);
    assert(BatteryGuard::message(BatteryAlert::Critical8).find("AutoRCM") != std::string::npos);
}

int main() {
    test_scenes();
    test_batterie();
    std::puts("scenes et batterie : tous les tests passent");
    return 0;
}
