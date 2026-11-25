// HomeNX+ : mise à jour depuis GitHub et captures d'un jeu (Switch uniquement).
#include "hnp/update.hpp"

#include <nlohmann/json.hpp>
#include <curl/curl.h>
#include <switch.h>

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <sstream>

namespace hnp {
namespace {

constexpr const char* kRelease = "https://api.github.com/repos/kerobero0311/HomeNXPlus/releases/latest";
constexpr const char* kCaInfo = "sdmc:/config/homenxplus/cacert.pem";

size_t toString(char* p, size_t s, size_t n, void* u) { static_cast<std::string*>(u)->append(p, s * n); return s * n; }
size_t toFile(char* p, size_t s, size_t n, void* u) { return std::fwrite(p, s, n, static_cast<FILE*>(u)) * s; }

CURL* prepare(const std::string& url) {
    CURL* c = curl_easy_init();
    if (!c) return nullptr;
    curl_easy_setopt(c, CURLOPT_URL, url.c_str());
    curl_easy_setopt(c, CURLOPT_USERAGENT, "HomeNXPlus-updater");
    curl_easy_setopt(c, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(c, CURLOPT_CAINFO, kCaInfo);
    curl_easy_setopt(c, CURLOPT_CONNECTTIMEOUT, 8L);
    curl_easy_setopt(c, CURLOPT_NOSIGNAL, 1L);
    return c;
}

std::vector<int> parts(std::string v) {
    if (!v.empty() && (v[0] == 'v' || v[0] == 'V')) v.erase(0, 1);
    std::vector<int> out;
    std::stringstream ss(v);
    std::string x;
    while (std::getline(ss, x, '.')) out.push_back(std::atoi(x.c_str()));
    return out;
}

} // namespace

int compareVersions(const std::string& a, const std::string& b) {
    const auto pa = parts(a), pb = parts(b);
    for (size_t i = 0; i < std::max(pa.size(), pb.size()); ++i) {
        const int x = i < pa.size() ? pa[i] : 0, y = i < pb.size() ? pb[i] : 0;
        if (x != y) return x < y ? -1 : 1;
    }
    return 0;
}

UpdateInfo checkUpdate() {
    UpdateInfo info;
    std::string body;
    CURL* c = prepare(kRelease);
    if (!c) { info.error = "Réseau indisponible."; return info; }
    curl_slist* h = curl_slist_append(nullptr, "Accept: application/vnd.github+json");
    curl_easy_setopt(c, CURLOPT_HTTPHEADER, h);
    curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, toString);
    curl_easy_setopt(c, CURLOPT_WRITEDATA, &body);
    curl_easy_setopt(c, CURLOPT_TIMEOUT, 20L);
    long code = 0;
    const CURLcode rc = curl_easy_perform(c);
    if (rc == CURLE_OK) curl_easy_getinfo(c, CURLINFO_RESPONSE_CODE, &code);
    curl_slist_free_all(h);
    curl_easy_cleanup(c);
    if (rc != CURLE_OK) { info.error = "Pas de connexion à GitHub (Wi-Fi ?)."; return info; }
    if (code == 404) { info.error = "Aucune version publiée pour l'instant."; return info; }
    if (code != 200) { info.error = "GitHub a répondu une erreur (" + std::to_string(code) + ")."; return info; }
    const auto j = nlohmann::json::parse(body, nullptr, false);
    if (j.is_discarded() || !j.is_object()) { info.error = "Réponse de GitHub illisible."; return info; }
    info.version = j.value("tag_name", "");
    if (!info.version.empty() && (info.version[0] == 'v' || info.version[0] == 'V')) info.version.erase(0, 1);
    info.notes = j.value("body", "").substr(0, 1500);
    if (j.contains("assets") && j["assets"].is_array())
        for (const auto& a : j["assets"])
            if (a.is_object() && a.value("name", "") == "HomeNXPlus.nro") info.url = a.value("browser_download_url", "");
    if (info.version.empty()) { info.error = "Version introuvable."; return info; }
    info.ok = true;
    return info;
}

bool downloadNro(const std::string& url, const std::string& dest, std::string& message) {
    if (url.rfind("https://github.com/", 0) != 0 && url.rfind("https://objects.githubusercontent.com/", 0) != 0) {
        message = "Adresse de téléchargement refusée."; return false;
    }
    const std::string part = dest + ".part";
    FILE* f = std::fopen(part.c_str(), "wb");
    if (!f) { message = "Impossible d'écrire sur la carte SD."; return false; }
    CURL* c = prepare(url);
    if (!c) { std::fclose(f); message = "Réseau indisponible."; return false; }
    curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, toFile);
    curl_easy_setopt(c, CURLOPT_WRITEDATA, f);
    curl_easy_setopt(c, CURLOPT_TIMEOUT, 600L);
    long code = 0;
    const CURLcode rc = curl_easy_perform(c);
    if (rc == CURLE_OK) curl_easy_getinfo(c, CURLINFO_RESPONSE_CODE, &code);
    curl_easy_cleanup(c);
    std::fclose(f);
    std::error_code ec;
    if (rc != CURLE_OK || code != 200) { std::filesystem::remove(part, ec); message = "Téléchargement interrompu."; return false; }
    // Un vrai .nro : « NRO0 » à l'octet 0x10, et une taille raisonnable
    char magic[4] = {};
    if (FILE* r = std::fopen(part.c_str(), "rb")) { std::fseek(r, 0x10, SEEK_SET); std::fread(magic, 1, 4, r); std::fclose(r); }
    const auto size = std::filesystem::file_size(part, ec);
    if (ec || size < 1'000'000 || std::string(magic, 4) != "NRO0") { std::filesystem::remove(part, ec); message = "Le fichier reçu n'est pas une appli Switch valide."; return false; }
    std::filesystem::rename(part, dest, ec);
    if (ec) { std::filesystem::remove(dest, ec); std::filesystem::rename(part, dest, ec); }
    if (ec) { message = "Impossible de remplacer l'ancienne version."; return false; }
    message = "Mise à jour installée : relance HomeNX+ pour l'utiliser.";
    return true;
}

std::vector<std::string> capturesOfGame(std::uint64_t titleId, int max) {
    std::vector<std::string> out;
    if (R_FAILED(capsaInitialize())) return out;
    std::vector<CapsAlbumEntry> all;
    for (CapsAlbumStorage st : {CapsAlbumStorage_Sd, CapsAlbumStorage_Nand}) {
        u64 count = 0;
        if (R_FAILED(capsaGetAlbumFileCount(st, &count)) || count == 0) continue;
        std::vector<CapsAlbumEntry> l(count);
        u64 got = 0;
        if (R_FAILED(capsaGetAlbumFileList(st, &got, l.data(), count))) continue;
        for (u64 i = 0; i < got; ++i)
            if (l[i].file_id.application_id == titleId) all.push_back(l[i]);
    }
    capsaExit();
    auto key = [](const CapsAlbumEntry& e) { const auto& d = e.file_id.datetime;
        return ((((u64)d.year * 100 + d.month) * 100 + d.day) * 100 + d.hour) * 10000ULL + d.minute * 100ULL + d.second; };
    std::sort(all.begin(), all.end(), [&](const CapsAlbumEntry& a, const CapsAlbumEntry& b) { return key(a) > key(b); });
    for (const auto& e : all) {
        if ((int)out.size() >= max) break;
        const auto& d = e.file_id.datetime;
        char b[64];
        std::snprintf(b, sizeof b, "%02u/%02u/%04u %02u:%02u · %s", d.day, d.month, d.year, d.hour, d.minute,
                      e.file_id.content == CapsAlbumFileContents_ScreenShot ? "photo" : "vidéo");
        out.push_back(b);
    }
    return out;
}

} // namespace hnp
