// libclaude : envoi d'une requete en streaming (Switch : libcurl + mbedTLS).
#pragma once
#include "libclaude/claude.hpp"

namespace claude {

struct HttpResult {
    long status = 0;      // code HTTP (0 si pas de reponse)
    int netError = 0;     // code libcurl (0 = OK)
    std::string errorBody;// corps JSON d'une erreur HTTP (jamais journalise avec la cle)
};

// Envoie la requete et appelle onEvent pour chaque evenement SSE recu.
// onEvent renvoie false pour annuler (bouton B). La cle n'est jamais ecrite nulle part.
HttpResult postStream(const std::string& apiKey, const std::string& body, const std::string& caPath,
                      const std::function<bool(const SseEvent&)>& onEvent, long timeoutSec = 120);

} // namespace claude
