// HomeNX+ : lien PC (configuration, Wake-on-LAN).
#include "hnp/pclink.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <sstream>

namespace hnp {

static std::string trim(std::string_view s) {
    std::size_t a = 0, b = s.size();
    while (a < b && std::isspace((unsigned char)s[a])) ++a;
    while (b > a && std::isspace((unsigned char)s[b - 1])) --b;
    return std::string(s.substr(a, b - a));
}

PcLink parsePcLink(std::string_view ini) {
    PcLink p;
    std::istringstream in{std::string(ini)};
    std::string line;
    while (std::getline(in, line)) {
        std::string l = trim(line);
        if (l.empty() || l[0] == ';' || l[0] == '#' || l[0] == '[') continue;
        auto eq = l.find('=');
        if (eq == std::string::npos) continue;
        std::string k = trim(std::string_view(l).substr(0, eq)), v = trim(std::string_view(l).substr(eq + 1));
        if (k == "ip") p.ip = v;
        else if (k == "port") { int x = std::atoi(v.c_str()); if (x >= 1024 && x <= 65535) p.port = x; }
        else if (k == "jeton") p.token = v;
        else if (k == "mac") p.mac = v;
    }
    return p;
}

std::optional<std::array<std::uint8_t, 6>> parseMac(std::string_view s) {
    std::array<std::uint8_t, 6> m{};
    std::string t = trim(s);
    if (t.size() != 17) return std::nullopt;
    for (int i = 0; i < 6; ++i) {
        if (i < 5 && t[i * 3 + 2] != ':' && t[i * 3 + 2] != '-') return std::nullopt;
        int v = 0;
        for (int k = 0; k < 2; ++k) {
            char c = (char)std::toupper((unsigned char)t[i * 3 + k]);
            int d = (c >= '0' && c <= '9') ? c - '0' : (c >= 'A' && c <= 'F') ? c - 'A' + 10 : -1;
            if (d < 0) return std::nullopt;
            v = v * 16 + d;
        }
        m[i] = (std::uint8_t)v;
    }
    return m;
}

std::vector<std::uint8_t> magicPacket(const std::array<std::uint8_t, 6>& mac) {
    std::vector<std::uint8_t> p(6, 0xFF);
    for (int i = 0; i < 16; ++i) p.insert(p.end(), mac.begin(), mac.end());
    return p;
}

std::string broadcastFor(const std::string& ip) {
    auto pos = ip.rfind('.');
    if (pos == std::string::npos || std::count(ip.begin(), ip.end(), '.') != 3) return "255.255.255.255";
    return ip.substr(0, pos) + ".255";
}

} // namespace hnp
