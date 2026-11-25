// Requete HTTPS en streaming vers l'API Anthropic (libcurl + mbedTLS sur Switch).
#include "libclaude/client.hpp"

#include <curl/curl.h>

namespace claude {
namespace {

struct Ctx {
    SseParser parser;
    const std::function<bool(const SseEvent&)>* onEvent = nullptr;
    long status = 0;
    CURL* curl = nullptr;
    std::string errorBody;
    bool cancelled = false;
};

size_t onData(char* ptr, size_t size, size_t nmemb, void* user) {
    auto* c = static_cast<Ctx*>(user);
    const size_t n = size * nmemb;
    if (c->status == 0) curl_easy_getinfo(c->curl, CURLINFO_RESPONSE_CODE, &c->status);
    if (c->status >= 400) {  // erreur HTTP : corps JSON simple, pas de SSE
        if (c->errorBody.size() < 16384) c->errorBody.append(ptr, n);
        return n;
    }
    for (const auto& ev : c->parser.feed(std::string_view(ptr, n))) {
        if (!(*c->onEvent)(ev)) { c->cancelled = true; return 0; }  // 0 = annule le transfert
    }
    return n;
}

} // namespace

HttpResult postStream(const std::string& apiKey, const std::string& body, const std::string& caPath,
                      const std::function<bool(const SseEvent&)>& onEvent, long timeoutSec) {
    HttpResult r;
    CURL* curl = curl_easy_init();
    if (!curl) { r.netError = CURLE_FAILED_INIT; return r; }

    Ctx ctx; ctx.onEvent = &onEvent; ctx.curl = curl;
    const std::string keyHeader = "x-api-key: " + apiKey;
    const std::string verHeader = std::string("anthropic-version: ") + kApiVersion;
    curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "content-type: application/json");
    headers = curl_slist_append(headers, "accept: text/event-stream");
    headers = curl_slist_append(headers, keyHeader.c_str());
    headers = curl_slist_append(headers, verHeader.c_str());

    curl_easy_setopt(curl, CURLOPT_URL, kApiUrl);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)body.size());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, onData);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ctx);
    curl_easy_setopt(curl, CURLOPT_CAINFO, caPath.c_str());
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeoutSec);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "ClaudeNX/0.1 (HomeNX+)");
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);  // jamais de trace : elle contiendrait la cle

    CURLcode rc = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &r.status);
    if (rc != CURLE_OK && !(ctx.cancelled && rc == CURLE_WRITE_ERROR)) r.netError = (int)rc;
    r.errorBody = std::move(ctx.errorBody);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return r;
}

} // namespace claude
