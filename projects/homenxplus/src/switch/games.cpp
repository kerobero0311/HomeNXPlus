// HomeNX+ : jeux installes, temps de jeu et dernier jeu joue (ns + pdm, lecture seule). Switch uniquement.
#include "hnp/switch_games.hpp"

#include <switch.h>

#include <cstdio>
#include <memory>

namespace hnp {

std::string gameName(std::uint64_t tid) {
    auto data = std::make_unique<NsApplicationControlData>();
    u64 size = 0;
    if (R_SUCCEEDED(nsGetApplicationControlData(NsApplicationControlSource_Storage, tid, data.get(), sizeof(*data), &size))) {
        NacpLanguageEntry* e = nullptr;
        if (R_SUCCEEDED(nacpGetLanguageEntry(&data->nacp, &e)) && e && e->name[0]) return e->name;
    }
    char b[17]; std::snprintf(b, sizeof b, "%016lX", (unsigned long)tid);
    return b;
}

std::vector<Item> installedGames(std::size_t max) {
    std::vector<Item> out;
    const bool pdm = R_SUCCEEDED(pdmqryInitialize());
    std::vector<NsApplicationRecord> recs(64);
    s32 offset = 0;
    while (out.size() < max) {
        s32 n = 0;
        if (R_FAILED(nsListApplicationRecord(recs.data(), (s32)recs.size(), offset, &n)) || n <= 0) break;
        for (s32 i = 0; i < n && out.size() < max; ++i) {
            Item it;
            const u64 tid = recs[i].application_id;
            char b[17]; std::snprintf(b, sizeof b, "%016lX", (unsigned long)tid);
            it.id = b; it.title = gameName(tid); it.platform = Platform::Switch;
            if (pdm) {
                PdmPlayStatistics st{};
                if (R_SUCCEEDED(pdmqryQueryPlayStatisticsByApplicationId(tid, false, &st))) {
                    it.playMinutes = (std::int64_t)(st.playtime / 60000000000ULL);
                    it.lastPlayed = (std::int64_t)st.last_timestamp_user;
                    it.launches = (int)st.total_launches;
                }
            }
            out.push_back(it);
        }
        offset += n;
    }
    if (pdm) pdmqryExit();
    return out;
}

std::uint64_t lastPlayedGame() {
    AccountUid uid{};
    if (R_FAILED(accountGetLastOpenedUser(&uid)) || !accountUidIsValid(&uid)) return 0;
    if (R_FAILED(pdmqryInitialize())) return 0;
    u64 ids[4] = {}; s32 total = 0;
    Result rc = pdmqryQueryRecentlyPlayedApplication(uid, false, ids, 4, &total);
    pdmqryExit();
    return R_SUCCEEDED(rc) && total > 0 ? ids[0] : 0;
}

} // namespace hnp
