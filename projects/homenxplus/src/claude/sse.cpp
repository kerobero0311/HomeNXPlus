// Analyse SSE et reconstruction du message en streaming.
#include "libclaude/claude.hpp"

namespace claude {

std::vector<SseEvent> SseParser::feed(std::string_view chunk) {
    std::vector<SseEvent> out;
    for (char c : chunk) {
        if (c != '\n') { m_line.push_back(c); continue; }
        std::string line = std::move(m_line);
        m_line.clear();
        if (!line.empty() && line.back() == '\r') line.pop_back();

        if (line.empty()) {  // ligne vide = fin d'un evenement
            if (m_hasData || !m_event.empty()) out.push_back({m_event, m_data});
            m_event.clear(); m_data.clear(); m_hasData = false;
            continue;
        }
        if (line[0] == ':') continue;  // commentaire (keep-alive)
        auto colon = line.find(':');
        std::string field = line.substr(0, colon);
        std::string value = colon == std::string::npos ? std::string() : line.substr(colon + 1);
        if (!value.empty() && value[0] == ' ') value.erase(0, 1);
        if (field == "event") m_event = value;
        else if (field == "data") {
            if (m_hasData) m_data.push_back('\n');
            m_data += value; m_hasData = true;
        }
    }
    return out;
}

static void readUsage(const json& u, Usage& usage) {
    if (!u.is_object()) return;
    auto get = [&](const char* k, std::int64_t& dst) { if (u.contains(k) && u[k].is_number_integer()) dst = u[k].get<std::int64_t>(); };
    get("input_tokens", usage.input);
    get("output_tokens", usage.output);
    get("cache_read_input_tokens", usage.cacheRead);
    get("cache_creation_input_tokens", usage.cacheWrite);
}

std::string StreamState::apply(const SseEvent& ev) {
    if (ev.data.empty()) return {};
    json d = json::parse(ev.data, nullptr, false);
    if (d.is_discarded() || !d.is_object()) return {};
    const std::string type = d.value("type", ev.event);

    if (type == "message_start") {
        if (d.contains("message")) readUsage(d["message"].value("usage", json::object()), m_usage);
    } else if (type == "content_block_start") {
        int i = d.value("index", 0);
        while ((int)m_content.size() <= i) m_content.push_back(json::object());
        m_content[i] = d.value("content_block", json::object());
    } else if (type == "content_block_delta") {
        int i = d.value("index", 0);
        while ((int)m_content.size() <= i) m_content.push_back(json::object());
        json& block = m_content[i];
        const json delta = d.value("delta", json::object());
        const std::string dt = delta.value("type", "");
        if (dt == "text_delta") {
            std::string t = delta.value("text", "");
            block["text"] = block.value("text", "") + t;
            m_text += t;
            return t;
        } else if (dt == "thinking_delta") {
            block["thinking"] = block.value("thinking", "") + delta.value("thinking", "");
        } else if (dt == "signature_delta") {
            block["signature"] = delta.value("signature", "");
        } else if (dt == "input_json_delta") {
            m_partialJson[i] += delta.value("partial_json", "");
        }
    } else if (type == "content_block_stop") {
        int i = d.value("index", 0);
        auto it = m_partialJson.find(i);
        if (it != m_partialJson.end() && i < (int)m_content.size()) {
            json input = json::parse(it->second.empty() ? "{}" : it->second, nullptr, false);
            m_content[i]["input"] = input.is_discarded() ? json::object() : input;
            m_partialJson.erase(it);
        }
    } else if (type == "message_delta") {
        const json delta = d.value("delta", json::object());
        if (delta.contains("stop_reason") && delta["stop_reason"].is_string()) m_stop = delta["stop_reason"];
        readUsage(d.value("usage", json::object()), m_usage);
    } else if (type == "message_stop") {
        m_done = true;
    } else if (type == "error") {
        const json e = d.value("error", json::object());
        m_errorType = e.value("type", "error");
        m_errorMessage = e.value("message", "");
        m_done = true;
    }
    return {};
}

} // namespace claude
