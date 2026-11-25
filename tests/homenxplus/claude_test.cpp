// Tests PC de libclaude : SSE, reconstruction du message, couts, config, erreurs.
#include "libclaude/claude.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <string>

using namespace claude;

static void test_sse_morceaux() {
    // Flux decoupe n'importe ou, avec \r\n et un commentaire keep-alive
    const std::string flux =
        ": ping\r\n\r\n"
        "event: message_start\r\ndata: {\"type\":\"message_start\",\"message\":{\"usage\":{\"input_tokens\":120,\"output_tokens\":1,\"cache_read_input_tokens\":0}}}\r\n\r\n"
        "event: content_block_start\ndata: {\"type\":\"content_block_start\",\"index\":0,\"content_block\":{\"type\":\"thinking\",\"thinking\":\"\"}}\n\n"
        "event: content_block_delta\ndata: {\"type\":\"content_block_delta\",\"index\":0,\"delta\":{\"type\":\"signature_delta\",\"signature\":\"SIG\"}}\n\n"
        "event: content_block_stop\ndata: {\"type\":\"content_block_stop\",\"index\":0}\n\n"
        "event: content_block_start\ndata: {\"type\":\"content_block_start\",\"index\":1,\"content_block\":{\"type\":\"text\",\"text\":\"\"}}\n\n"
        "event: content_block_delta\ndata: {\"type\":\"content_block_delta\",\"index\":1,\"delta\":{\"type\":\"text_delta\",\"text\":\"Bonjour \"}}\n\n"
        "event: content_block_delta\ndata: {\"type\":\"content_block_delta\",\"index\":1,\"delta\":{\"type\":\"text_delta\",\"text\":\"Link !\"}}\n\n"
        "event: content_block_stop\ndata: {\"type\":\"content_block_stop\",\"index\":1}\n\n"
        "event: message_delta\ndata: {\"type\":\"message_delta\",\"delta\":{\"stop_reason\":\"end_turn\"},\"usage\":{\"output_tokens\":42}}\n\n"
        "event: message_stop\ndata: {\"type\":\"message_stop\"}\n\n";
    for (std::size_t taille : {1u, 3u, 7u, 64u, 100000u}) {
        SseParser p; StreamState s; std::string vu;
        for (std::size_t i = 0; i < flux.size(); i += taille)
            for (auto& ev : p.feed(std::string_view(flux).substr(i, taille))) vu += s.apply(ev);
        assert(vu == "Bonjour Link !");
        assert(s.text() == "Bonjour Link !");
        assert(s.done() && !s.failed());
        assert(s.stopReason() == "end_turn");
        assert(s.usage().input == 120 && s.usage().output == 42);
        assert(s.content().size() == 2);
        assert(s.content()[0]["type"] == "thinking" && s.content()[0]["signature"] == "SIG");  // renvoye tel quel
        assert(s.content()[1]["text"] == "Bonjour Link !");
    }
}

static void test_sse_erreur_et_outil() {
    SseParser p; StreamState s;
    std::string f = "event: content_block_start\ndata: {\"type\":\"content_block_start\",\"index\":0,\"content_block\":{\"type\":\"tool_use\",\"id\":\"t1\",\"name\":\"appliquer_scene\",\"input\":{}}}\n\n"
                    "event: content_block_delta\ndata: {\"type\":\"content_block_delta\",\"index\":0,\"delta\":{\"type\":\"input_json_delta\",\"partial_json\":\"{\\\"nom\\\":\"}}\n\n"
                    "event: content_block_delta\ndata: {\"type\":\"content_block_delta\",\"index\":0,\"delta\":{\"type\":\"input_json_delta\",\"partial_json\":\"\\\"Nuit\\\"}\"}}\n\n"
                    "event: content_block_stop\ndata: {\"type\":\"content_block_stop\",\"index\":0}\n\n"
                    "event: error\ndata: {\"type\":\"error\",\"error\":{\"type\":\"overloaded_error\",\"message\":\"Overloaded\"}}\n\n";
    for (auto& ev : p.feed(f)) s.apply(ev);
    assert(s.content()[0]["input"]["nom"] == "Nuit");
    assert(s.failed() && s.errorType() == "overloaded_error");
    assert(errorMessageFr(0, s.errorType(), 0).find("surcharge") != std::string::npos);
    // JSON abime : ignore sans planter
    StreamState s2; s2.apply({"content_block_delta", "{pas du json"}); assert(!s2.failed() && s2.text().empty());
}

static void test_requete() {
    RequestOptions o; o.system = "Tu es l'assistant de HomeNX+.";
    std::vector<Message> h{{"user", "Salut"}, {"assistant", json::array({{{"type", "text"}, {"text", "Bonjour"}}})}, {"user", "Ca va ?"}};
    json b = json::parse(buildRequestBody(o, h));
    assert(b["model"] == kModelHaiku && b["stream"] == true && b["max_tokens"] == 4096);
    assert(b["output_config"]["effort"] == "low");
    assert(!b.contains("thinking") && !b.contains("temperature"));  // interdits / inutiles sur les modeles actuels
    assert(b["messages"].size() == 3 && b["messages"][1]["content"][0]["text"] == "Bonjour");
    assert(b["system"] == "Tu es l'assistant de HomeNX+.");
    const std::uint8_t d[] = {'M', 'a', 'n'};
    assert(base64Encode(d, 3) == "TWFu" && base64Encode(d, 2) == "TWE=" && base64Encode(d, 1) == "TQ==");
    json img = imageBlockJpeg({0xFF, 0xD8, 0xFF});
    assert(img["source"]["media_type"] == "image/jpeg" && img["source"]["data"] == "/9j/");
}

static void test_couts() {
    Usage u; u.input = 1000000; u.output = 1000000;
    assert(std::fabs(costUsd(kModelOpus, u) - 24.0) < 1e-9);
    Usage h; h.input = 50000; h.output = 2000;  // petite question sur Haiku
    assert(std::fabs(costUsd(kModelHaiku, h) - (50000 * 0.10 + 2000 * 0.50) / 1e6) < 1e-12);
    Usage g; g.input = 150000;                   // au-dela de 100K : tarif majore
    assert(std::fabs(costUsd(kModelHaiku, g) - 150000 * 0.50 / 1e6) < 1e-12);
    assert(std::fabs(costUsd("modele-inconnu", u) - 24.0) < 1e-9);  // jamais sous-estime
    std::string csv = ledgerLine("2026-11-25T10:00", kModelHaiku, h, 0.006) + ledgerLine("2026-11-25T10:05", kModelOpus, u, 1.5) + "ligne abimee\n";
    assert(std::fabs(ledgerTotalUsd(csv) - 1.506) < 1e-9);
    assert(!capReached(4.0, 5.0, 0.92) && capReached(5.5, 5.0, 0.92) && capReached(0, 0, 0.92));
}

static void test_config_et_cle() {
    Config c = parseConfig("[claude]\n; commentaire\nmodel = claude-opus-5-5\nmax_tokens=99999999\neffort = high\nmonthly_cap_eur = 2.5\nusd_to_eur=abc\n");
    assert(c.model == "claude-opus-5-5" && c.maxTokens == 4096 && c.effort == "high" && c.monthlyCapEur == 2.5 && c.usdToEur == 0.92);
    assert(sanitizeApiKey("  sk-ant-api03-abcdefghijklmnop\r\n") == "sk-ant-api03-abcdefghijklmnop");
    assert(sanitizeApiKey("ma cle").empty() && sanitizeApiKey("sk-ant-api03-ab cd-efghijkl").empty());
}

static void test_textes() {
    assert(asciiFold("Évolué, ça marche à 100 % — “oui” …") == "Evolue, ca marche a 100 % - \"oui\" ...");
    assert(asciiFold("cœur") == "coeur");
    assert(asciiFold(std::string("coupe\xC3", 6)) == "coupe");  // sequence tronquee ignoree
    assert(errorMessageFr(401, "authentication_error", 0).find("Cle API") != std::string::npos);
    assert(errorMessageFr(0, "", 6).find("Wi-Fi") != std::string::npos);
    assert(stopReasonFr("max_tokens").find("coupee") != std::string::npos && stopReasonFr("end_turn").empty());
}

int main() {
    test_sse_morceaux();
    test_sse_erreur_et_outil();
    test_requete();
    test_couts();
    test_config_et_cle();
    test_textes();
    std::puts("libclaude : tous les tests passent");
    return 0;
}
