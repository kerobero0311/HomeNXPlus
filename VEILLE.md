# VEILLE — projets existants (relevé du 24/11/2026)

Sources : API GitHub (dépôts, versions, licences, README) et GameBrew pour ChatNX et sLaunch.

## Base du menu HOME

| Projet | Licence | Dernière version | Activité | Ce qu'on en reprend |
|---|---|---|---|---|
| **ad3333m/homenx** (HomeNX + CardsNX + NXToggle) | GPL-3.0 | build 212 (stable), 213 (bêta du 15/11) | très actif (push le 24/11/2026), 185 étoiles, 369 commits | **BASE DU FORK** : c'est le menu installé sur la console (build 199). Il intègre déjà ses propres correctifs de stabilité : watchdog, démarrage de secours après un crash de CardsNX, 64 Mo laissés libres pour les apps, fonds SteamGridDB libérés |
| PoloNX/SwitchU | GPL-3.0 | 1.4.2 (20/11/2026) | actif, 304 étoiles | projet d'origine de HomeNX. On suivra ses correctifs du daemon et du menu (cherry-pick si utile) |
| tomvita/SwitchU | GPL-3.0 (fork de PoloNX) | 1.2.0o (24/10/2026), série 1.2.0k → o | modéré, 4 étoiles | **source de correctifs** : watchdog 4 s, service `avm`, crash en mode applet, jeux qui quittent après le logo, tri « Récents » persistant, **filtre par nom + clavier USB** (absent de HomeNX), L puis Y pour le hbmenu, titres CJK |
| XorTroll/uLaunch | **GPL-2.0** (sans « ou ultérieure » d'après l'API) | 1.2.6 (15/11/2026) | actif, 816 étoiles | **inspiration seulement** : raccourcis .nro sans forwarder, thèmes, gestion du bouton HOME. Une GPL-2.0 seule n'est pas compatible avec la GPL-3.0 : pas de copie de code |
| etonedemid/slaunch (sLaunch) | GPL-3.0 (« ou ultérieure », d'après l'en-tête de LICENSE ; GitHub affiche « Other ») | v2.5.2 (19/11/2026) | actif, 74 étoiles | inspiration : modes d'interface, widgets Lua, packs d'icônes. Code réutilisable s'il est bien en GPL-3.0+ (en-têtes à vérifier fichier par fichier) |

## Claude / IA sur Switch

| Projet | Licence | Dernière activité | Notes |
|---|---|---|---|
| zerob13/pocket-deepchat | MIT | 26/07/2026 | client IA sous PocketJS (moteur JavaScript), qui gère Anthropic. Streaming, sessions, clavier système. On s'inspire de l'interface de chat ; code non réutilisable tel quel (JavaScript, pas C++) |
| Jonathan8520/ChatNX | non précisée par GitHub (MIT d'après GameBrew) | 09/09/2026 | `.nro` C++ / SDL2 / libcurl via la passerelle Puter. Analyse de capture, suivi des coûts par modèle, 28 langues. Source d'idées pour l'envoi de captures et l'affichage du coût. **Pas de passerelle tierce chez nous** : API Anthropic directe |

## Discord

| Projet | Licence | Version | Notes |
|---|---|---|---|
| SunResearchInstitute/SwitchPresence-Rewritten | GPL-2.0 | v1.8.2 (2020), dépôt poussé le 20/11/2026 | sysmodule + PresenceClient sur le PC. **À installer tel quel** (phase 11, avec accord). Vieille version : vérifier la compatibilité avec le firmware 23 avant d'installer |
| Grarak/NXCord | MIT | 02/11/2026 | **EXCLU** (règle 2.7 : client Discord non officiel) |

## ⚠️ Constat important : client Discord intégré à HomeNX
HomeNX contient lui-même un **client Discord non officiel** (`projects/menu/src/discord/`, connexion par QR code ou jeton, bouton L). Son propre README prévient que Discord peut signaler les comptes qui utilisent un client tiers.
→ **Conservé dans HomeNX+ à la demande de l'utilisateur** (24/11/2026, risque accepté) ; voir DECISIONS D7.
