// HomeNX+ : services de films, séries et musique sans appli Switch.
// Le PC les ouvre (pc_agent, liste fermée identique dans pc_agent/agent.py), Moonlight les affiche.
// Une seule liste pour la page Médias, le panneau HomeNX+ et l'apparence Wii U.
#pragma once

#include <cstddef>

namespace hnp {

struct StreamingService {
    const char* id;     // identifiant envoyé au PC (doit exister dans SERVICES de pc_agent)
    const char* name;   // nom affiché
    const char* about;  // une ligne sous le nom
    bool music;         // icône musique plutôt que lecture
};

inline constexpr StreamingService kStreamingServices[] = {
    {"netflix", "Netflix", "Films et séries", false},
    {"disney", "Disney+", "Disney, Pixar, Marvel, Star Wars", false},
    {"prime", "Prime Video", "Films et séries Amazon", false},
    {"youtube", "YouTube (PC)", "Ton compte et tes abonnements", false},
    {"twitch", "Twitch", "Directs de jeux", false},
    {"crunchyroll", "Crunchyroll", "Animés", false},
    {"adn", "ADN", "Animés en français", false},
    {"canal", "myCanal", "Chaînes et séries Canal+", false},
    {"francetv", "france.tv", "Direct et replay, gratuit", false},
    {"arte", "Arte.tv", "Films et documentaires, gratuit", false},
    {"max", "Max", "Séries HBO", false},
    {"appletv", "Apple TV+", "Séries Apple", false},
    {"paramount", "Paramount+", "Films et séries Paramount", false},
    {"spotify", "Spotify", "Musique", true},
    {"deezer", "Deezer", "Musique", true},
    {"applemusic", "Apple Music", "Musique", true},
};
inline constexpr std::size_t kStreamingCount = sizeof(kStreamingServices) / sizeof(kStreamingServices[0]);

} // namespace hnp
