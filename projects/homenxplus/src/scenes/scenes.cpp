// HomeNX+ : implementation des scenes et de la garde batterie.
#include "hnp/scenes.hpp"

#include <algorithm>

namespace hnp {

// Frequences officielles Erista (meme liste que le config.ini de sys-clk de l'utilisateur)
static const int kCpu[] = {612, 714, 816, 918, 1020, 1122, 1224, 1326, 1428, 1581, 1683, 1785};
static const int kGpu[] = {76, 153, 230, 307, 384, 460, 537, 614, 691, 768, 844, 921};
static const int kMem[] = {665, 800, 1065, 1331, 1600};

template <std::size_t N> static bool inList(const int (&l)[N], int v) { return v == 0 || std::find(std::begin(l), std::end(l), v) != std::end(l); }
bool isValidCpu(int mhz) { return inList(kCpu, mhz); }
bool isValidGpu(int mhz) { return inList(kGpu, mhz); }
bool isValidMem(int mhz) { return inList(kMem, mhz); }

Scenes::Scenes() {
    Scene nuit;  nuit.id = "nuit";  nuit.name = "Nuit";     nuit.colorTempK = 3400; nuit.fizeauOn = true; nuit.brightness = 0.25; nuit.builtIn = true;
    Scene batt;  batt.id = "batterie"; batt.name = "Batterie"; batt.cpuMHz = 1020; batt.gpuMHz = 307; batt.memMHz = 1331; batt.brightness = 0.35; batt.builtIn = true;
    Scene perf;  perf.id = "perf";  perf.name = "Perf max"; perf.cpuMHz = 1785; perf.gpuMHz = 921; perf.memMHz = 1600; perf.builtIn = true;
    m_list = {nuit, batt, perf};
}

const Scene* Scenes::find(const std::string& id) const {
    for (const auto& s : m_list) if (s.id == id) return &s;
    return nullptr;
}

bool Scenes::upsert(const Scene& s) {
    if (s.id.empty() || s.name.empty()) return false;
    if (!isValidCpu(s.cpuMHz) || !isValidGpu(s.gpuMHz) || !isValidMem(s.memMHz)) return false;
    if (s.colorTempK != 0 && (s.colorTempK < 1000 || s.colorTempK > 10000)) return false;
    if (s.brightness != -1 && (s.brightness < 0.0 || s.brightness > 1.0)) return false;
    for (auto& x : m_list) if (x.id == s.id) { bool b = x.builtIn; x = s; x.builtIn = b; return true; }
    Scene c = s; c.builtIn = false; m_list.push_back(c);
    return true;
}

bool Scenes::remove(const std::string& id) {
    auto it = std::find_if(m_list.begin(), m_list.end(), [&](const Scene& s) { return s.id == id; });
    if (it == m_list.end() || it->builtIn) return false;
    m_list.erase(it);
    for (auto i = m_perGame.begin(); i != m_perGame.end();) i = i->second == id ? m_perGame.erase(i) : std::next(i);
    if (active == id) active.clear();
    return true;
}

void Scenes::setForGame(const std::string& titleId, const std::string& sceneId) {
    if (sceneId.empty() || !find(sceneId)) m_perGame.erase(titleId); else m_perGame[titleId] = sceneId;
}

std::optional<std::string> Scenes::forGame(const std::string& titleId) const {
    auto it = m_perGame.find(titleId);
    if (it == m_perGame.end() || !find(it->second)) return std::nullopt;
    return it->second;
}

json Scenes::toJson() const {
    json j; j["version"] = 1; j["active"] = active; j["perGame"] = m_perGame; j["scenes"] = json::array();
    for (const auto& s : m_list)
        j["scenes"].push_back({{"id", s.id}, {"name", s.name}, {"cpu", s.cpuMHz}, {"gpu", s.gpuMHz}, {"mem", s.memMHz},
                               {"colorTempK", s.colorTempK}, {"fizeau", s.fizeauOn}, {"brightness", s.brightness}, {"builtIn", s.builtIn}});
    return j;
}

Scenes Scenes::fromJson(const json& j) {
    Scenes r;  // repart des scenes fournies, puis applique le fichier (valeurs verifiees)
    if (!j.is_object()) return r;
    for (const auto& x : j.value("scenes", json::array())) {
        if (!x.is_object()) continue;
        Scene s; s.id = x.value("id", ""); s.name = x.value("name", "");
        s.cpuMHz = x.value("cpu", 0); s.gpuMHz = x.value("gpu", 0); s.memMHz = x.value("mem", 0);
        s.colorTempK = x.value("colorTempK", 0); s.fizeauOn = x.value("fizeau", false); s.brightness = x.value("brightness", -1.0);
        r.upsert(s);  // une scene invalide (frequence inconnue...) est ignoree
    }
    if (j.contains("perGame") && j["perGame"].is_object())
        for (auto& [k, v] : j["perGame"].items()) if (v.is_string()) r.setForGame(k, v);
    std::string a = j.value("active", "");
    if (r.find(a)) r.active = a;
    return r;
}

// ---------------------------------------------------------------- garde batterie
BatteryAlert BatteryGuard::update(int percent, bool charging) {
    BatteryAlert level = BatteryAlert::None;
    if (!charging) {
        if (percent <= 8) level = BatteryAlert::Critical8;
        else if (percent <= 15) level = BatteryAlert::Low15;
    }
    // On rearme les alertes quand la batterie remonte nettement (hysteresis de 5 %) ou qu'on branche le chargeur
    if (charging || percent >= 20) { m_last = BatteryAlert::None; return BatteryAlert::None; }
    if ((int)level > (int)m_last) { m_last = level; return level; }
    return BatteryAlert::None;
}

std::string BatteryGuard::message(BatteryAlert a) {
    switch (a) {
        case BatteryAlert::Low15:
            return "Batterie a 15 % : branche le chargeur. Avec l'AutoRCM, une batterie vide oblige a reinjecter Hekate depuis le PC.";
        case BatteryAlert::Critical8:
            return "Batterie critique (8 %) : branche le chargeur MAINTENANT, ou mets la console en veille. Batterie vide = reinjection obligatoire (AutoRCM).";
        default: return {};
    }
}

} // namespace hnp
