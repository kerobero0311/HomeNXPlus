// HomeNX+ : actions reelles sur la console (Switch uniquement).
#pragma once

#include "hnp/scenes.hpp"

#include <cstdint>
#include <string>

namespace hnp {

struct BatteryStatus {
    int percent = -1;      // -1 = inconnu
    bool charging = false;
};

// Applique une scene : frequences via sys-clk (si le sysmodule tourne), luminosite via lbl.
// Renvoie un compte rendu en francais (ce qui a ete applique ou non). Ne coupe jamais la console.
std::string applyScene(const Scene& s);
// Retire les forcages de frequence de HomeNX+ (sys-clk reprend ses profils).
std::string clearScene();

BatteryStatus readBattery();
bool sysclkRunning();

// --- lien avec le PC (pc_agent) ; textes de retour en francais
struct PcLink;
std::string wakePc(const PcLink& pc);                         // paquet Wake-on-LAN (UDP 9, diffusion)
bool pingPc(const PcLink& pc);                                // le pc_agent repond-il ?
std::string sendToPc(const PcLink& pc, const std::string& text);
std::string readFromPc(const PcLink& pc, int& count);         // messages en attente (texte concatene)
std::string sendCapturesToPc(const PcLink& pc, int max);     // captures pas encore envoyees, les plus recentes
// Streaming : demande au PC d'ouvrir "netflix", "disney", "prime" ou "spotify" (liste fermee cote PC).
// Reveille le PC et l'attend (60 s au plus) s'il ne repond pas. true si le site est ouvert.
bool openOnPc(const PcLink& pc, const std::string& service, std::string& message);
std::string sendSavesToPc(const PcLink& pc, std::uint64_t titleId, const std::string& gameName);  // lecture seule

} // namespace hnp
