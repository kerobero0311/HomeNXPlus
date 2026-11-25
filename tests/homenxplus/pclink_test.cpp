// Tests PC du lien PC / Wake-on-LAN.
#include "hnp/pclink.hpp"

#include <cassert>
#include <cstdio>

using namespace hnp;

int main() {
    PcLink p = parsePcLink("; pc_agent\nip = 192.168.1.20\nport=9000\njeton = abcdefghijklmnopqrstuvwxyz0123456789\nmac=aa:bb:cc:dd:ee:ff\n");
    assert(p.configured() && p.ip == "192.168.1.20" && p.port == 9000 && p.mac == "aa:bb:cc:dd:ee:ff");
    assert(!parsePcLink("ip=1.2.3.4\n").configured());                 // sans jeton : pas configure
    assert(parsePcLink("port=80\n").port == 8642);                       // port refuse
    auto m = parseMac("AA-BB-CC-DD-EE-0f");
    assert(m && (*m)[0] == 0xAA && (*m)[5] == 0x0F);
    assert(!parseMac("AA:BB:CC:DD:EE") && !parseMac("GG:BB:CC:DD:EE:FF") && !parseMac("AABBCCDDEEFF00000"));
    auto pkt = magicPacket(*m);
    assert(pkt.size() == 102 && pkt[0] == 0xFF && pkt[5] == 0xFF && pkt[6] == 0xAA && pkt[101] == 0x0F);
    assert(broadcastFor("192.168.1.50") == "192.168.1.255" && broadcastFor("rien") == "255.255.255.255");
    std::puts("lien PC : tous les tests passent");
    return 0;
}
