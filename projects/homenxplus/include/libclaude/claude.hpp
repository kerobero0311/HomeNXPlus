// libclaude : client minimal de l'API Messages d'Anthropic pour HomeNX+ et ClaudeNX.
// API HTTP directe (pas de SDK officiel en C++ pour la Switch), en streaming (SSE).
// Tout ce fichier est de la logique pure, testable sur PC (tests/homenxplus/).
// La partie reseau (libcurl + mbedTLS) est dans src/switch/client.cpp.
#pragma once

#include <nlohmann/json.hpp>

#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace claude {

using json = nlohmann::json;

// --- Constantes de l'API (verifiees le 25/11/2026 dans la documentation Anthropic) ---
inline constexpr const char* kApiUrl      = "https://api.anthropic.com/v1/messages";
inline constexpr const char* kApiVersion  = "2026-10-21";
inline constexpr const char* kModelHaiku  = "claude-haiku-5-5";   // par defaut : rapide et economique
inline constexpr const char* kModelOpus   = "claude-opus-5-5";    // option : le plus capable
inline constexpr const char* kModelSonnet = "claude-sonnet-5-5";

// ---------------------------------------------------------------- SSE
struct SseEvent {
    std::string event;  // nom de l'evenement (ligne "event:")
    std::string data;   // donnees (lignes "data:" concatenees)
};

// Decoupe un flux Server-Sent Events arrivant par morceaux de taille quelconque.
class SseParser {
public:
    std::vector<SseEvent> feed(std::string_view chunk);
private:
    std::string m_line;
    std::string m_event;
    std::string m_data;
    bool m_hasData = false;
};

// ---------------------------------------------------------------- reponse en streaming
struct Usage {
    std::int64_t input = 0;        // jetons d'entree factures plein tarif
    std::int64_t output = 0;       // jetons de sortie (cumules par message_delta)
    std::int64_t cacheRead = 0;    // jetons lus depuis le cache
    std::int64_t cacheWrite = 0;   // jetons ecrits dans le cache
};

// Reconstruit le message de l'assistant a partir des evenements SSE.
class StreamState {
public:
    // Applique un evenement ; renvoie le texte visible ajoute (a afficher tout de suite).
    std::string apply(const SseEvent& ev);

    const std::string& text() const { return m_text; }        // texte visible complet
    const json& content() const { return m_content; }          // blocs a renvoyer tels quels dans l'historique
    const std::string& stopReason() const { return m_stop; }   // end_turn, max_tokens, refusal, tool_use...
    const Usage& usage() const { return m_usage; }
    bool done() const { return m_done; }
    bool failed() const { return !m_errorType.empty(); }
    const std::string& errorType() const { return m_errorType; }
    const std::string& errorMessage() const { return m_errorMessage; }

private:
    std::string m_text;
    json m_content = json::array();
    std::map<int, std::string> m_partialJson;  // entrees d'outils en cours (input_json_delta)
    std::string m_stop;
    Usage m_usage;
    bool m_done = false;
    std::string m_errorType;
    std::string m_errorMessage;
};

// ---------------------------------------------------------------- requete
struct Message {
    std::string role;  // "user" ou "assistant"
    json content;      // chaine simple ou tableau de blocs
};

struct RequestOptions {
    std::string model = kModelHaiku;
    int maxTokens = 4096;
    std::string effort = "low";  // low / medium / high ; low suffit pour le chat sur console
    std::string system;          // instructions systeme (vide = aucune)
};

std::string buildRequestBody(const RequestOptions& opt, const std::vector<Message>& messages);

// Bloc image (JPEG d'une capture de l'Album) pour la vision.
json imageBlockJpeg(const std::vector<std::uint8_t>& jpeg);
std::string base64Encode(const std::uint8_t* data, std::size_t size);

// ---------------------------------------------------------------- couts
struct Price {
    double inputPerMTok;
    double outputPerMTok;
    double cacheReadPerMTok;
    double cacheWritePerMTok;
};
// Tarifs publics en dollars par million de jetons (releve du 03/11/2026).
Price priceFor(const std::string& model, std::int64_t promptTokens);
double costUsd(const std::string& model, const Usage& u);

// Registre mensuel : une ligne CSV par reponse "date;modele;entree;sortie;cache_lu;cache_ecrit;cout_usd".
std::string ledgerLine(const std::string& isoDate, const std::string& model, const Usage& u, double usd);
double ledgerTotalUsd(const std::string& csv);
bool capReached(double spentUsd, double capEur, double usdToEur);

// ---------------------------------------------------------------- configuration
struct Config {
    std::string model = kModelHaiku;
    std::string modelAlt = kModelOpus;
    int maxTokens = 4096;
    std::string effort = "low";
    double monthlyCapEur = 5.0;   // plafond bloquant
    double usdToEur = 0.92;       // conversion indicative
    std::string system;           // instructions systeme perso (facultatif)
};
// Lit un config.ini simple (cle = valeur, lignes ; ou # ignorees, sections ignorees).
Config parseConfig(std::string_view ini);
// Nettoie une cle API lue dans un fichier (espaces, fin de ligne). Vide si invalide.
std::string sanitizeApiKey(std::string_view raw);

// ---------------------------------------------------------------- erreurs en francais
// httpStatus : 0 si la requete n'a pas abouti ; netError : code libcurl (0 = OK).
std::string errorMessageFr(long httpStatus, const std::string& apiErrorType, int netError);
std::string stopReasonFr(const std::string& stopReason);  // vide si fin normale

// Texte UTF-8 -> ASCII pour la console de libnx (accents retires : e, a, c...).
std::string asciiFold(std::string_view utf8);

} // namespace claude
