// HomeNX+ : lien avec le PC (pc_agent) et reveil du PC (Wake-on-LAN). Logique pure, testable.
// Reglages sur la carte : sdmc:/config/homenxplus/pc_agent.ini
//   ip = 192.168.1.20      port = 8642      jeton = ...      mac = AA:BB:CC:DD:EE:FF
#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace hnp {

struct PcLink {
    std::string ip;
    int port = 8642;
    std::string token;   // jamais affiche ni journalise
    std::string mac;     // pour le Wake-on-LAN
    bool configured() const { return !ip.empty() && !token.empty(); }
};

PcLink parsePcLink(std::string_view ini);

// "AA:BB:CC:DD:EE:FF" ou "AA-BB-..." -> 6 octets
std::optional<std::array<std::uint8_t, 6>> parseMac(std::string_view s);
// Paquet magique Wake-on-LAN : 6 x 0xFF puis 16 fois l'adresse MAC (102 octets)
std::vector<std::uint8_t> magicPacket(const std::array<std::uint8_t, 6>& mac);
// Adresse de diffusion d'un reseau /24 a partir d'une IP ("192.168.1.20" -> "192.168.1.255")
std::string broadcastFor(const std::string& ip);

} // namespace hnp
