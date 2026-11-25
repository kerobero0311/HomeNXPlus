// HomeNX+ : actions vers le PC (Wake-on-LAN en UDP, pc_agent en HTTP). Switch uniquement.
#include "hnp/pclink.hpp"
#include "hnp/switch_actions.hpp"

#include <nlohmann/json.hpp>
#include <curl/curl.h>
#include <switch.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <cstdio>
#include <vector>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace hnp {
namespace {

size_t collect(char* p, size_t s, size_t n, void* u) { static_cast<std::string*>(u)->append(p, s * n); return s * n; }

// Requete vers le pc_agent ; renvoie le code HTTP (0 = pas de reponse)
long call(const PcLink& pc, const char* method, const std::string& path, const std::string& body, std::string& out) {
    CURL* c = curl_easy_init();
    if (!c) return 0;
    const std::string url = "http://" + pc.ip + ":" + std::to_string(pc.port) + path;
    const std::string tok = "X-HNP-Jeton: " + pc.token;
    curl_slist* h = curl_slist_append(nullptr, tok.c_str());
    h = curl_slist_append(h, "Content-Type: application/json");
    curl_easy_setopt(c, CURLOPT_URL, url.c_str());
    curl_easy_setopt(c, CURLOPT_HTTPHEADER, h);
    curl_easy_setopt(c, CURLOPT_CUSTOMREQUEST, method);
    if (!body.empty()) {  // corps binaire possible (captures JPEG) : taille explicite
        curl_easy_setopt(c, CURLOPT_POSTFIELDS, body.data());
        curl_easy_setopt(c, CURLOPT_POSTFIELDSIZE_LARGE, (curl_off_t)body.size());
    }
    curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, collect);
    curl_easy_setopt(c, CURLOPT_WRITEDATA, &out);
    curl_easy_setopt(c, CURLOPT_CONNECTTIMEOUT, 3L);
    curl_easy_setopt(c, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(c, CURLOPT_NOSIGNAL, 1L);
    long code = 0;
    if (curl_easy_perform(c) == CURLE_OK) curl_easy_getinfo(c, CURLINFO_RESPONSE_CODE, &code);
    curl_slist_free_all(h);
    curl_easy_cleanup(c);
    return code;
}

} // namespace

std::string wakePc(const PcLink& pc) {
    auto mac = parseMac(pc.mac);
    if (!mac) return "Adresse MAC du PC absente ou invalide (mac = dans config/homenxplus/pc_agent.ini).";
    const auto pkt = magicPacket(*mac);
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) return "Reseau indisponible (Wi-Fi ?).";
    int yes = 1;
    setsockopt(s, SOL_SOCKET, SO_BROADCAST, &yes, sizeof(yes));
    sockaddr_in a{};
    a.sin_family = AF_INET;
    a.sin_port = htons(9);
    int sent = 0;
    for (const std::string& dst : {broadcastFor(pc.ip), std::string("255.255.255.255")}) {
        inet_pton(AF_INET, dst.c_str(), &a.sin_addr);
        if (sendto(s, pkt.data(), pkt.size(), 0, (sockaddr*)&a, sizeof(a)) == (ssize_t)pkt.size()) ++sent;
    }
    close(s);
    return sent ? "Signal de reveil envoye au PC. Il demarre en general en 10 a 30 secondes "
                  "(le Wake-on-LAN doit etre active dans le BIOS et la carte reseau du PC)."
                : "Impossible d'envoyer le signal de reveil.";
}

bool pingPc(const PcLink& pc) {
    if (pc.ip.empty()) return false;
    std::string out;
    return call(pc, "GET", "/ping", "", out) == 200;
}

std::string sendToPc(const PcLink& pc, const std::string& text) {
    if (!pc.configured()) return "Le lien avec le PC n'est pas configure (config/homenxplus/pc_agent.ini).";
    std::string out;
    long code = call(pc, "POST", "/boite", nlohmann::json{{"texte", text}}.dump(), out);
    if (code == 201) return "Message envoye au PC.";
    if (code == 401) return "Le PC refuse : jeton different (refais l'installation sur la carte).";
    if (code == 0) return "Le PC ne repond pas : pc_agent est-il lance ? Meme reseau Wi-Fi ?";
    return "Erreur du PC (HTTP " + std::to_string(code) + ").";
}

// Envoie au PC les captures d'ecran de l'Album pas encore envoyees (les plus recentes d'abord, `max` au plus).
// Lecture seule de l'Album ; la liste des envois est gardee dans sdmc:/homenxplus/pc_captures_envoyees.txt
std::string sendCapturesToPc(const PcLink& pc, int max) {
    if (!pc.configured()) return "Le lien avec le PC n'est pas configure.";
    if (!pingPc(pc)) return "Le PC ne repond pas : lance pc_agent sur le PC (meme reseau Wi-Fi).";
    const char* journal = "sdmc:/homenxplus/pc_captures_envoyees.txt";
    std::string deja;
    { FILE* f = std::fopen(journal, "rb"); if (f) { char b[4096]; size_t n; while ((n = std::fread(b, 1, sizeof b, f)) > 0) deja.append(b, n); std::fclose(f); } }

    if (R_FAILED(capsaInitialize())) return "Album inaccessible.";
    std::vector<CapsAlbumEntry> toutes;
    for (CapsAlbumStorage st : {CapsAlbumStorage_Sd, CapsAlbumStorage_Nand}) {
        u64 count = 0;
        if (R_FAILED(capsaGetAlbumFileCount(st, &count)) || count == 0) continue;
        std::vector<CapsAlbumEntry> l(count); u64 got = 0;
        if (R_FAILED(capsaGetAlbumFileList(st, &got, l.data(), count))) continue;
        for (u64 i = 0; i < got; ++i) if (l[i].file_id.content == CapsAlbumFileContents_ScreenShot) toutes.push_back(l[i]);
    }
    auto cle = [](const CapsAlbumEntry& e) { const auto& d = e.file_id.datetime;
        char b[48]; std::snprintf(b, sizeof b, "%04u%02u%02u-%02u%02u%02u-%02u-%016lX", d.year, d.month, d.day, d.hour, d.minute, d.second, d.id,
                                  (unsigned long)e.file_id.application_id); return std::string(b); };
    std::sort(toutes.begin(), toutes.end(), [&](const CapsAlbumEntry& a, const CapsAlbumEntry& b) { return cle(a) > cle(b); });

    int envoyees = 0, erreurs = 0;
    for (const auto& e : toutes) {
        if (envoyees >= max) break;
        const std::string nom = cle(e) + ".jpg";
        if (deja.find(nom) != std::string::npos) continue;
        u64 size = 0, out = 0;
        if (R_FAILED(capsaGetAlbumFileSize(&e.file_id, &size)) || size == 0 || size > 8 * 1024 * 1024) { ++erreurs; continue; }
        std::string data(size, '\0');
        if (R_FAILED(capsaLoadAlbumFile(&e.file_id, &out, data.data(), size))) { ++erreurs; continue; }
        data.resize(out);
        std::string rep;
        if (call(pc, "PUT", "/depot/captures/" + nom, data, rep) == 201) {
            ++envoyees;
            FILE* f = std::fopen(journal, "ab"); if (f) { std::fputs((nom + "\n").c_str(), f); std::fclose(f); }
        } else ++erreurs;
    }
    capsaExit();
    if (!envoyees && !erreurs) return "Aucune nouvelle capture a envoyer.";
    return std::to_string(envoyees) + " capture(s) envoyee(s) dans Documents\\Switch_Sauvegardes\\<date>\\captures"
           + (erreurs ? " (" + std::to_string(erreurs) + " echec(s))." : ".");
}

// Copie sur le PC la sauvegarde d'un jeu (utilisateur courant). On ne fait que LIRE la sauvegarde :
// elle est montee, ses fichiers sont lus un par un et envoyes, puis elle est demontee.
std::string sendSavesToPc(const PcLink& pc, std::uint64_t tid, const std::string& gameName) {
    if (!pc.configured()) return "Le lien avec le PC n'est pas configure.";
    if (!pingPc(pc)) return "Le PC ne repond pas : lance pc_agent sur le PC (meme reseau Wi-Fi).";
    AccountUid uid{};
    if (R_FAILED(accountGetLastOpenedUser(&uid)) || !accountUidIsValid(&uid)) return "Aucun utilisateur trouve.";
    if (R_FAILED(fsdevMountSaveData("hnpsave", tid, uid))) return "Pas de sauvegarde pour ce jeu (ou jeu en cours : ferme-le d'abord).";

    auto clean = [](std::string s) {  // noms acceptes par pc_agent : lettres, chiffres, espace _ - . ( ) [ ]
        for (char& c : s) if (!(std::isalnum((unsigned char)c) || c == ' ' || c == '_' || c == '-' || c == '.' || c == '(' || c == ')' || c == '[' || c == ']')) c = '_';
        return s;
    };
    const std::string prefix = clean(gameName).substr(0, 40);
    int ok = 0, ko = 0;
    std::error_code ec;
    for (auto it = std::filesystem::recursive_directory_iterator("hnpsave:/", ec);
         !ec && it != std::filesystem::recursive_directory_iterator(); it.increment(ec)) {
        if (!it->is_regular_file(ec)) continue;
        std::string rel = it->path().string();
        auto pos = rel.find(":/"); if (pos != std::string::npos) rel = rel.substr(pos + 2);
        for (char& c : rel) if (c == '/') c = '~';  // dossiers aplatis : a~b.dat
        std::string nom = prefix + "__" + clean(rel);
        if (nom.size() > 120) nom = nom.substr(nom.size() - 120);
        std::string data;
        { FILE* f = std::fopen(it->path().c_str(), "rb"); if (!f) { ++ko; continue; }
          char b[65536]; size_t n; while ((n = std::fread(b, 1, sizeof b, f)) > 0) data.append(b, n); std::fclose(f); }
        if (data.empty() || data.size() > 64 * 1024 * 1024) { ++ko; continue; }
        std::string out;
        std::string url;
        for (unsigned char c : nom) { if (c == ' ') url += "%20"; else url += (char)c; }
        if (call(pc, "PUT", "/depot/saves/" + url, data, out) == 201) ++ok; else ++ko;
    }
    fsdevUnmountDevice("hnpsave");
    if (!ok && !ko) return "La sauvegarde est vide.";
    return std::to_string(ok) + " fichier(s) de sauvegarde copie(s) dans Documents\\Switch_Sauvegardes\\<date>\\saves"
           + (ko ? " (" + std::to_string(ko) + " echec(s))." : ".");
}

bool openOnPc(const PcLink& pc, const std::string& service, std::string& message) {
    if (!pc.configured()) { message = "Le lien avec le PC n'est pas configure (config/homenxplus/pc_agent.ini)."; return false; }
    if (!pingPc(pc)) {
        if (pc.mac.empty()) { message = "Le PC ne repond pas : allume-le et lance pc_agent."; return false; }
        wakePc(pc);
        bool up = false;
        for (int i = 0; i < 20 && !up; ++i) {  // ~60 s : demarrage du PC puis de pc_agent
            svcSleepThread(3'000'000'000LL);
            up = pingPc(pc);
        }
        if (!up) { message = "Le PC ne s'est pas reveille (Wake-on-LAN actif ? pc_agent au demarrage ?)."; return false; }
    }
    std::string out;
    const long code = call(pc, "POST", "/ouvrir", nlohmann::json{{"service", service}}.dump(), out);
    if (code == 200) { message = "Ouvert sur le PC."; return true; }
    if (code == 404) message = "pc_agent est trop ancien sur le PC : mets-le a jour (version 0.2.0).";
    else if (code == 401) message = "Le PC refuse : jeton different (refais l'installation sur la carte).";
    else if (code == 0) message = "Le PC ne repond pas.";
    else message = "Le PC n'a pas pu ouvrir le service (HTTP " + std::to_string(code) + ").";
    return false;
}

std::string readFromPc(const PcLink& pc, int& count) {
    count = 0;
    if (!pc.configured()) return "";
    std::string out;
    if (call(pc, "GET", "/boite", "", out) != 200) return "";
    auto j = nlohmann::json::parse(out, nullptr, false);
    if (j.is_discarded() || !j.contains("messages") || !j["messages"].is_array()) return "";
    std::string all;
    for (const auto& m : j["messages"]) {
        if (!m.is_object()) continue;
        ++count;
        all += m.value("date", "") + " : " + m.value("texte", "") + "\n";
    }
    return all;
}

} // namespace hnp
