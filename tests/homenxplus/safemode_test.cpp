// Tests PC du compteur de mode sans echec.
#include "hnp/safemode.hpp"

#include <cassert>
#include <cstdio>
#include <filesystem>
#include <fstream>

int main() {
    namespace fs = std::filesystem;
    const std::string p = (fs::temp_directory_path() / "hnp_starts_test").string();
    fs::remove(p);
    using namespace hnp;
    assert(beginStart(p) == 0 && !isSafe(0));          // 1er demarrage
    assert(beginStart(p) == 1);                          // plantage 1
    assert(beginStart(p) == 2);                          // plantage 2
    int n = beginStart(p);                               // plantage 3 -> mode sans echec
    assert(n == 3 && isSafe(n) && g_failedStarts == 3);
    markHealthy(p);                                      // il a tenu 30 s
    assert(beginStart(p) == 0);
    { std::ofstream(p) << "n'importe quoi"; }            // fichier abime : on repart de 0
    assert(beginStart(p) == 0);
    { std::ofstream(p) << -5; }
    assert(beginStart(p) == 0);
    fs::remove(p);
    std::puts("mode sans echec : tous les tests passent");
    return 0;
}
