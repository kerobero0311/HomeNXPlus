// HomeNX+ : compteur de demarrages pour le mode sans echec.
#include "hnp/safemode.hpp"

#include <fstream>

namespace hnp {

bool g_safeMode = false;
int g_failedStarts = 0;

int beginStart(const std::string& path) {
    int n = 0;
    { std::ifstream in(path); if (!(in >> n) || n < 0 || n > 1000) n = 0; }
    { std::ofstream out(path, std::ios::trunc); out << (n + 1); }
    g_failedStarts = n;
    return n;
}

void markHealthy(const std::string& path) {
    std::ofstream out(path, std::ios::trunc);
    out << 0;
}

} // namespace hnp
