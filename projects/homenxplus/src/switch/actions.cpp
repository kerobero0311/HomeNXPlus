// HomeNX+ : application des scenes (sys-clk, luminosite) et lecture de la batterie.
// Client sys-clk ecrit pour HomeNX+ d'apres le protocole public de sys-clk (retronx-team/sys-clk,
// common/include/sysclk/ipc.h) : service "sys:clk", commande 8 = SetOverride {module, hz}, hz 0 = retire.
#include "hnp/switch_actions.hpp"

#include <switch.h>

#include <cstdio>
#include <functional>

namespace hnp {
namespace {

constexpr u32 kCmdGetApiVersion = 0;
constexpr u32 kCmdSetOverride = 8;
enum : u32 { kModCpu = 0, kModGpu = 1, kModMem = 2 };

struct OverrideArgs { u32 module; u32 hz; };

bool withSysclk(const std::function<void(Service&)>& fn) {
    Service s{};
    if (R_FAILED(smGetService(&s, "sys:clk"))) return false;
    fn(s);
    serviceClose(&s);
    return true;
}

bool setFizeau(bool on) {
    Service s{};
    if (R_FAILED(smGetService(&s, "fizeau"))) return false;
    const u8 v = on ? 1 : 0;
    const bool ok = R_SUCCEEDED(serviceDispatchIn(&s, 1, v));  // FizeauCommandId_SetIsActive
    serviceClose(&s);
    return ok;
}

} // namespace

bool sysclkRunning() {
    bool ok = false;
    withSysclk([&](Service& s) { u32 v = 0; ok = R_SUCCEEDED(serviceDispatchOut(&s, kCmdGetApiVersion, v)); });
    return ok;
}

static std::string setClocks(int cpu, int gpu, int mem) {
    int fails = 0;
    bool reached = withSysclk([&](Service& s) {
        const OverrideArgs a[] = {{kModCpu, (u32)cpu * 1000000u}, {kModGpu, (u32)gpu * 1000000u}, {kModMem, (u32)mem * 1000000u}};
        for (const auto& x : a) if (R_FAILED(serviceDispatchIn(&s, kCmdSetOverride, x))) ++fails;
    });
    if (!reached) return "sys-clk ne repond pas (frequences inchangees). ";
    if (fails) return "sys-clk a refuse une frequence. ";
    if (!cpu && !gpu && !mem) return "Frequences rendues a sys-clk. ";
    return "Frequences appliquees. ";
}

std::string applyScene(const Scene& sc) {
    std::string out = setClocks(sc.cpuMHz, sc.gpuMHz, sc.memMHz);  // 0 = retire le forcage (sys-clk decide)
    if (sc.brightness >= 0) {
        if (R_SUCCEEDED(lblInitialize())) {
            lblSetCurrentBrightnessSetting((float)sc.brightness);
            lblExit();
            out += "Luminosite reglee. ";
        } else out += "Luminosite non modifiable ici. ";
    }
    // Fizeau (protocole public d'averne/Fizeau : service "fizeau", commande 1 = SetIsActive) : on allume
    // le filtre avec le profil choisi dans Fizeau ; on ne l'eteint que si c'est HomeNX+ qui l'a allume.
    const char* flag = "sdmc:/homenxplus/fizeau_par_hnp";
    if (sc.fizeauOn) {
        if (setFizeau(true)) { FILE* f = std::fopen(flag, "w"); if (f) std::fclose(f); out += "Filtre Fizeau allume. "; }
        else out += "Fizeau ne repond pas (sysmodule arrete ?). ";
    } else if (FILE* f = std::fopen(flag, "r")) {
        std::fclose(f);
        if (setFizeau(false)) { std::remove(flag); out += "Filtre Fizeau eteint. "; }
    }
    return out;
}

std::string clearScene() {
    std::string out = setClocks(0, 0, 0);
    if (FILE* f = std::fopen("sdmc:/homenxplus/fizeau_par_hnp", "r")) {
        std::fclose(f);
        if (setFizeau(false)) { std::remove("sdmc:/homenxplus/fizeau_par_hnp"); out += "Filtre Fizeau eteint. "; }
    }
    return out;
}

BatteryStatus readBattery() {
    BatteryStatus b;
    if (R_FAILED(psmInitialize())) return b;
    u32 pct = 0;
    if (R_SUCCEEDED(psmGetBatteryChargePercentage(&pct))) b.percent = (int)pct;
    PsmChargerType ct{};
    if (R_SUCCEEDED(psmGetChargerType(&ct))) b.charging = ct != PsmChargerType_Unconnected;
    psmExit();
    return b;
}

} // namespace hnp
