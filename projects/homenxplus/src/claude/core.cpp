// Requetes, couts, configuration, messages d'erreur et texte pour la console.
#include "libclaude/claude.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <sstream>

namespace claude {

// ---------------------------------------------------------------- requete
std::string buildRequestBody(const RequestOptions& opt, const std::vector<Message>& messages) {
    json body;
    body["model"] = opt.model;
    body["max_tokens"] = opt.maxTokens;
    body["stream"] = true;
    if (!opt.system.empty()) body["system"] = opt.system;
    // Reflexion adaptative active par defaut sur Haiku 5.5 / Opus 5.5 : on regle seulement l'effort.
    if (!opt.effort.empty()) body["output_config"] = {{"effort", opt.effort}};
    json msgs = json::array();
    for (const auto& m : messages) msgs.push_back({{"role", m.role}, {"content", m.content}});
    body["messages"] = msgs;
    return body.dump();
}

std::string base64Encode(const std::uint8_t* data, std::size_t size) {
    static const char* t = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve((size + 2) / 3 * 4);
    for (std::size_t i = 0; i < size; i += 3) {
        std::uint32_t n = std::uint32_t(data[i]) << 16;
        if (i + 1 < size) n |= std::uint32_t(data[i + 1]) << 8;
        if (i + 2 < size) n |= data[i + 2];
        out.push_back(t[(n >> 18) & 63]);
        out.push_back(t[(n >> 12) & 63]);
        out.push_back(i + 1 < size ? t[(n >> 6) & 63] : '=');
        out.push_back(i + 2 < size ? t[n & 63] : '=');
    }
    return out;
}

json imageBlockJpeg(const std::vector<std::uint8_t>& jpeg) {
    return {{"type", "image"},
            {"source", {{"type", "base64"}, {"media_type", "image/jpeg"}, {"data", base64Encode(jpeg.data(), jpeg.size())}}}};
}

// ---------------------------------------------------------------- couts
Price priceFor(const std::string& model, std::int64_t promptTokens) {
    if (model.rfind("claude-opus-5-5", 0) == 0)   return {4.00, 20.00, 0.20, 5.00};
    if (model.rfind("claude-sonnet-5-5", 0) == 0) return {2.00, 10.00, 0.10, 2.50};
    if (model.rfind("claude-haiku-5-5", 0) == 0) {
        // Haiku 5.5 : tarif plus eleve au-dela de 100K jetons de prompt
        if (promptTokens > 100000) return {0.50, 2.50, 0.05, 0.625};
        return {0.10, 0.50, 0.01, 0.125};
    }
    // Modele inconnu : on compte au tarif Opus pour ne jamais sous-estimer.
    return {4.00, 20.00, 0.20, 5.00};
}

double costUsd(const std::string& model, const Usage& u) {
    const std::int64_t prompt = u.input + u.cacheRead + u.cacheWrite;
    const Price p = priceFor(model, prompt);
    return (u.input * p.inputPerMTok + u.output * p.outputPerMTok +
            u.cacheRead * p.cacheReadPerMTok + u.cacheWrite * p.cacheWritePerMTok) / 1e6;
}

std::string ledgerLine(const std::string& isoDate, const std::string& model, const Usage& u, double usd) {
    char buf[256];
    std::snprintf(buf, sizeof(buf), "%s;%s;%lld;%lld;%lld;%lld;%.6f\n", isoDate.c_str(), model.c_str(),
                  (long long)u.input, (long long)u.output, (long long)u.cacheRead, (long long)u.cacheWrite, usd);
    return buf;
}

double ledgerTotalUsd(const std::string& csv) {
    double total = 0;
    std::istringstream in(csv);
    std::string line;
    while (std::getline(in, line)) {
        auto pos = line.rfind(';');
        if (pos == std::string::npos) continue;
        char* end = nullptr;
        double v = std::strtod(line.c_str() + pos + 1, &end);
        if (end != line.c_str() + pos + 1 && v >= 0) total += v;
    }
    return total;
}

bool capReached(double spentUsd, double capEur, double usdToEur) {
    if (capEur <= 0) return true;  // plafond nul = Claude coupe
    return spentUsd * usdToEur >= capEur;
}

// ---------------------------------------------------------------- configuration
static std::string trim(std::string_view s) {
    std::size_t a = 0, b = s.size();
    while (a < b && std::isspace((unsigned char)s[a])) ++a;
    while (b > a && std::isspace((unsigned char)s[b - 1])) --b;
    return std::string(s.substr(a, b - a));
}

Config parseConfig(std::string_view ini) {
    Config c;
    std::istringstream in{std::string(ini)};
    std::string line;
    while (std::getline(in, line)) {
        std::string l = trim(line);
        if (l.empty() || l[0] == ';' || l[0] == '#' || l[0] == '[') continue;
        auto eq = l.find('=');
        if (eq == std::string::npos) continue;
        std::string k = trim(std::string_view(l).substr(0, eq)), v = trim(std::string_view(l).substr(eq + 1));
        std::transform(k.begin(), k.end(), k.begin(), [](unsigned char ch) { return (char)std::tolower(ch); });
        auto num = [&](double def) { char* e = nullptr; double x = std::strtod(v.c_str(), &e); return e == v.c_str() ? def : x; };
        if (k == "model" && !v.empty()) c.model = v;
        else if (k == "model_alt" && !v.empty()) c.modelAlt = v;
        else if (k == "max_tokens") { int m = (int)num(c.maxTokens); if (m >= 256 && m <= 64000) c.maxTokens = m; }
        else if (k == "effort" && (v == "low" || v == "medium" || v == "high")) c.effort = v;
        else if (k == "monthly_cap_eur") { double x = num(c.monthlyCapEur); if (x >= 0) c.monthlyCapEur = x; }
        else if (k == "usd_to_eur") { double x = num(c.usdToEur); if (x > 0 && x < 10) c.usdToEur = x; }
        else if (k == "system") c.system = v;
    }
    return c;
}

std::string sanitizeApiKey(std::string_view raw) {
    std::string k = trim(raw);
    if (k.rfind("sk-ant-", 0) != 0 || k.size() < 20) return {};
    for (char ch : k) if (std::isspace((unsigned char)ch) || (unsigned char)ch < 0x21 || (unsigned char)ch > 0x7e) return {};
    return k;
}

// ---------------------------------------------------------------- erreurs
std::string errorMessageFr(long httpStatus, const std::string& t, int netError) {
    if (netError != 0) {
        switch (netError) {
            case 6:  return "Pas d'Internet : la console ne trouve pas api.anthropic.com. Verifie le Wi-Fi.";
            case 7:  return "Connexion impossible a api.anthropic.com. Verifie le Wi-Fi (ou 90DNS qui bloquerait le site).";
            case 28: return "Delai depasse : la reponse a mis trop de temps. Reessaie.";
            case 35: case 60: case 77:
                     return "Erreur de securite (certificat). Verifie que config/homenxplus/cacert.pem est present.";
            default: return "Erreur reseau (code " + std::to_string(netError) + "). Reessaie.";
        }
    }
    if (t == "authentication_error" || httpStatus == 401) return "Cle API invalide. Verifie config/homenxplus/api_key.txt.";
    if (t == "permission_error" || httpStatus == 403)     return "Cette cle n'a pas acces a ce modele.";
    if (t == "billing_error" || httpStatus == 402)        return "Credit API epuise : recharge sur console.anthropic.com.";
    if (t == "rate_limit_error" || httpStatus == 429)     return "Trop de demandes (quota). Attends un peu et reessaie.";
    if (t == "overloaded_error" || httpStatus == 529)     return "Claude est surcharge pour le moment. Reessaie dans quelques minutes.";
    if (t == "request_too_large" || httpStatus == 413)    return "Demande trop grosse (image ou historique trop long).";
    if (t == "not_found_error" || httpStatus == 404)      return "Modele introuvable : verifie le nom du modele dans config.ini.";
    if (t == "invalid_request_error" || httpStatus == 400) return "Demande refusee par l'API (parametre invalide).";
    if (httpStatus >= 500)                                return "Erreur du serveur Anthropic. Reessaie plus tard.";
    return "Erreur inattendue (HTTP " + std::to_string(httpStatus) + ").";
}

std::string stopReasonFr(const std::string& s) {
    if (s == "max_tokens") return "(Reponse coupee : limite de longueur atteinte.)";
    if (s == "refusal")    return "(Claude a refuse de repondre a cette demande.)";
    if (s == "pause_turn") return "(Reponse interrompue, relance la question.)";
    return {};
}

// ---------------------------------------------------------------- console
std::string asciiFold(std::string_view s) {
    std::string out;
    out.reserve(s.size());
    for (std::size_t i = 0; i < s.size();) {
        unsigned char c = (unsigned char)s[i];
        if (c < 0x80) { out.push_back((char)c); ++i; continue; }
        std::uint32_t cp = 0; int n = 0;
        if ((c & 0xE0) == 0xC0) { cp = c & 0x1F; n = 1; }
        else if ((c & 0xF0) == 0xE0) { cp = c & 0x0F; n = 2; }
        else if ((c & 0xF8) == 0xF0) { cp = c & 0x07; n = 3; }
        else { ++i; continue; }
        bool ok = true;  // sequence UTF-8 complete et valide ?
        for (int k = 1; ok && k <= n; ++k) {
            if (i + k >= s.size() || ((unsigned char)s[i + k] & 0xC0) != 0x80) { ok = false; break; }
            cp = (cp << 6) | ((unsigned char)s[i + k] & 0x3F);
        }
        i += ok ? n + 1 : 1;
        if (!ok) continue;
        const char* r = nullptr;
        switch (cp) {
            case 0xE0: case 0xE2: case 0xE4: r = "a"; break;  case 0xC0: case 0xC2: case 0xC4: r = "A"; break;
            case 0xE9: case 0xE8: case 0xEA: case 0xEB: r = "e"; break;  case 0xC9: case 0xC8: case 0xCA: case 0xCB: r = "E"; break;
            case 0xEE: case 0xEF: r = "i"; break;  case 0xCE: case 0xCF: r = "I"; break;
            case 0xF4: case 0xF6: r = "o"; break;  case 0xD4: case 0xD6: r = "O"; break;
            case 0xF9: case 0xFB: case 0xFC: r = "u"; break;  case 0xD9: case 0xDB: case 0xDC: r = "U"; break;
            case 0xE7: r = "c"; break;  case 0xC7: r = "C"; break;  case 0xFF: r = "y"; break;
            case 0x153: r = "oe"; break; case 0x152: r = "OE"; break; case 0xE6: r = "ae"; break;
            case 0x2019: case 0x2018: r = "'"; break;  case 0x201C: case 0x201D: case 0xAB: case 0xBB: r = "\""; break;
            case 0x2013: case 0x2014: r = "-"; break;  case 0x2026: r = "..."; break;  case 0xA0: case 0x202F: r = " "; break;
            case 0x20AC: r = "EUR"; break;  case 0x2022: r = "*"; break;
            default: r = "?"; break;
        }
        out += r;
    }
    return out;
}

} // namespace claude
