#pragma once
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <string>

namespace switchu::lock_screen {
inline bool enabled() {
    std::string frontend;
    std::getline(std::ifstream(HNP_CFG_SWITCHU "/frontend.txt"), frontend);
    if (frontend.rfind("cards", 0) == 0) {
        #ifdef HNP_CFG_CARDS
        std::ifstream in(HNP_CFG_CARDS "/settings.json");
#else
        std::ifstream in("sdmc:/config/CardsNX/settings.json");
#endif
        const auto settings = nlohmann::json::parse(in, nullptr, false);
        if (!settings.is_object()) return false;
        const auto at = settings.find("lockScreen");
        return at != settings.end() && at->is_boolean() && at->get<bool>();
    }
    std::error_code ec;
    return std::filesystem::exists(HNP_CFG_SWITCHU "/lock_screen", ec);
}
}
