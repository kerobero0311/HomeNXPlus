# Décisions techniques — HomeNX+

## D1 — Origine exacte du HomeNX installé (24/11/2026)

**Constat sur la carte SD** (lecture seule) :

| Indice | Valeur |
|---|---|
| `switch/SwitchU/homenx-build.txt` | `build 199` |
| `atmosphere/contents/0100000000001000/exefs.nsp` | 726 415 octets, SHA-256 `C2B03B1B3D4E88B1…` |
| `switch/SwitchU/bin/` | `menu/main` (11 Mo), `hbl/`, `cardsnx.nro` 1.2.0, `homenx-loader.nro` 1.2.0, `homenx-toggle.nro` (NXToggle 1.2.0), `homebrew-manager.nro`, `sphaira.nro`, `homenx-achievements.ovl` |
| `config/SwitchU/` | `daemon.log`, `menu.log`, `frontend.txt` (`cards`), `discord_hosts.txt`, `achievements.txt`, `applist.bin`… |
| Fonctions visibles | cartes CardsNX, apparence Wii U, Discord, lecteur de musique, succès, synthèse vocale (espeak-ng), shaders « liquid glass », HomeNX Loader à la place de `hbmenu.nro` |
| Paquet Ultrahand « HomeNX » | bascule `exefs.nsp` ↔ `exefs.nsp.disabled` et `frontend.txt` (même méthode que NXToggle) |

**Dépôts comparés** :

| Dépôt | Rôle | Licence | Activité |
|---|---|---|---|
| **ad3333m/homenx** | **HomeNX + CardsNX + NXToggle + HomeNX Loader**, exactement les noms et fichiers de la SD. Dernières versions : build 212 (stable), 213 (bêta du 15/11/2026) | GPL-3.0 | très actif, 185 étoiles, poussé le 24/11/2026 |
| PoloNX/SwitchU | projet d'origine (menu style Wii U), 1.4.2 du 20/11/2026. HomeNX en est une « version modifiée » (THIRD_PARTY_NOTICES) | GPL-3.0 | actif, 304 étoiles |
| tomvita/SwitchU | fork de PoloNX/SwitchU, sans CardsNX ni HomeNX | GPL-3.0 | dernier push le 24/10/2026 |
| ad3333m/frontend-injector | outil USB d'installation de HomeNX | — | annexe |

**Décision** : base = **ad3333m/homenx** (branche `master`).
- C'est la source exacte de la console (build 199) : même arborescence `switch/SwitchU/`, mêmes noms de binaires, même `config/SwitchU/`.
- Le dépôt est maintenu, compile avec **xmake** + devkitA64, et sait produire un `.nro` de test (`--homebrew=y`) sans toucher au vrai menu HOME.
- On passera directement à la dernière version (212 ou plus), pas à la 199.

**Licence** : GPL-3.0. HomeNX+ sera donc publié sous **GPL-3.0**, en gardant :
- les mentions de copyright des fichiers source ;
- `LICENSE` et `THIRD_PARTY_NOTICES.md`, plus une section « HomeNX+ est une version modifiée de HomeNX (ad3333m), elle-même tirée de SwitchU (PoloNX) ».

Le code source de toute version distribuée devra être publié.

## D2 — Structure du dépôt
- `HomeNXPlus/` = clone de `ad3333m/homenx`, avec en plus `pc_agent/`, `linux/`, les scripts `*.ps1` et les fichiers de pilotage.
- Remote `upstream` = ad3333m/homenx, pour récupérer ses correctifs. Notre travail se fait sur `main`.
- Le clone a lieu en phase 0. Les fichiers de pilotage créés aujourd'hui y seront déplacés.

## D3 — Taille (règle « moins de 60 Mo »)
Le HomeNX actuel occupe **96,4 Mo** dans `switch/SwitchU/` :

| Partie | Taille |
|---|---|
| `bin/` | 49,5 Mo (menu 11 Mo, CardsNX 13,5 Mo, NXToggle 10,5 Mo, Homebrew Manager 10,4 Mo, sphaira 5,4 Mo) |
| `sounds/` (dont 4 musiques MP3) | 24,7 Mo |
| `espeak-ng-data/` (voix) | 18,2 Mo |

Pour tenir sous 60 Mo, il faudra retirer ou rendre optionnels des éléments : voix espeak, musiques, copies d'apps déjà présentes ailleurs sur la SD (sphaira est déjà dans `switch/sphaira/`). **À décider avec l'utilisateur** (question 5 de la session 1).

## D4 — Environnement de build (constat)
- Git 2.55, GitHub CLI et Python 3.14 (lanceur `py`) sont installés. L'alias `python` du Microsoft Store n'est pas un vrai Python : utiliser `py`.
- ~~devkitPro incomplet~~ **Corrigé le 24/11** : devkitPro est bien installé, **à l'intérieur de MSYS2** (`C:\devkitPro\msys64\opt\devkitpro`). On y trouve devkitA64 r30, libnx 4.12.0, switch-tools (elf2nro, nacptool, nxlink…) et les dépôts `dkp-libs` et `dkp-windows` dans pacman. Variable à utiliser : `DEVKITPRO=C:/devkitPro/msys64/opt/devkitpro`.
- xmake **v3.1.2** installé en version portable dans `%USERPROFILE%\xmake` (sans installeur système, sans modifier le PATH global).
- Pas de WSL ni de Docker : on compile sous Windows natif avec le `build.ps1` d'origine de HomeNX, qui gère ce cas (voir `toolchain/devkitA64.lua`).
- Claude Code en ligne de commande : 2.1.331 dans `%USERPROFILE%\.local\bin\claude.exe` (pas dans le PATH ; `nuit.ps1` le trouve à cet endroit).

## D5 — Choix de démarrage (24/11/2026, « fais au mieux »)
Choix par défaut, faute de réponse précise, à revoir quand l'utilisateur le souhaite :

| Question | Choix | Pourquoi |
|---|---|---|
| Q1 Nom | **HomeNX+** (dossiers `HomeNXPlus`) | nom du cahier des charges |
| Q2 Style | **CardsNX sombre** (cartes sur verre dépoli) | c'est l'apparence utilisée aujourd'hui (`frontend.txt` = `cards`) |
| Q3 Budget API | **moins de 5 € par mois**, plafond bloquant | plus prudent ; modifiable dans `config.ini` |
| Q4 IP | PC sur le Wi-Fi de la maison (adresse dans pc_agent.ini) ; Switch : à lire au premier test | — |
| Q5 Taille | **option 3** : pas de voix espeak ni de musiques MP3 (seulement les petits sons), et réutiliser les apps déjà sur la SD (sphaira, NXToggle…) au lieu de les dupliquer | seule option compatible avec la limite de 60 Mo |
| Q6 Installations | faites de jour ; **pas de tâche planifiée** tant que l'heure n'est pas choisie | une tâche planifiée est un réglage système, qui demande un accord explicite |

## D6 — Base du fork confirmée après la veille (24/11/2026)
Le cahier des charges proposait tomvita/SwitchU en priorité. Après vérification, **la base reste ad3333m/homenx** :
1. C'est exactement le menu installé (build 199). On garde CardsNX, l'apparence utilisée, et rien n'est perdu du rangement.
2. Il est le plus actif (build 213, poussé le 24/11/2026) et contient déjà ses propres correctifs de stabilité (watchdog, démarrage de secours, réserve mémoire de 64 Mo).
3. tomvita/SwitchU est un fork de PoloNX **sans CardsNX**, avec 4 étoiles et un dernier push le 24/10. Partir de lui ferait perdre l'apparence actuelle.
→ En **phase 1**, on reprend de tomvita ce qui manque à HomeNX : filtre par nom avec clavier USB, vm, retour au menu après le logo d'un jeu, à vérifier un par un. Ce sont tous des projets GPL-3.0, donc compatibles.
- uLaunch (GPL-2.0 seule) : inspiration uniquement, aucun code copié.
- sLaunch (GPL-3.0+) : code réutilisable après vérification des en-têtes.
- Crédits prévus dans `CREDITS.md` : PoloNX (SwitchU), ad3333m (HomeNX), tomvita (correctifs), et les autres selon ce qui est repris.

## D7 — Client Discord de HomeNX : CONSERVÉ (décision de l'utilisateur, 24/11/2026)
HomeNX embarque un client Discord non officiel (connexion par QR code ou jeton, `projects/menu/src/discord/`, touche L). Proposition initiale : le retirer (ancienne règle 2.7). **L'utilisateur a choisi de le garder** : « c'est pas grave tant que ça fonctionne correctement sans problème de compatibilité ».
- Le module reste compilé et actif ; on vérifie qu'il marche avec libnx 4.12 et le firmware 23 (il compile déjà).
- Risque connu et accepté : Discord peut limiter ou fermer un compte utilisé avec un client tiers (TERMS.md de HomeNX).
- Le jeton est stocké en clair dans `config/SwitchU/discord.json` (PRIVACY.md de HomeNX). HomeNX+ ne le lit, ne le copie, ne le journalise et ne l'envoie jamais ailleurs que vers Discord (règle 3). `logs.ps1` ne rapatrie que les `*.log`.
- SwitchPresence-Rewritten (Rich Presence) reste possible en complément, en phase 11.

## D8 — Compilation sous Windows : ajustements nécessaires
- libnx 4.12 : `NacpStruct.lang` devient `lang_data.lang` (HomebrewStore.cpp).
- `packages/s/switch-pkg` : copie locale du modèle de PoloNX/switch-repo, qui ignore les bibliothèques debug (`nxd`, `deko3dd`, `drm_nouveaud`) quand la version normale existe.
- `xmake.lua` : `webp` relié après `webpdemux`.
- Données eSpeak : générées avec le MinGW de MSYS2 (`CMAKE_GENERATOR=MinGW Makefiles`).
- `lib/Atmosphere-libs` (daemon) : la version figée (16/06/2026) ne compile pas avec libnx 4.12 (API `pgl` renommée) ; passage au commit `75c68891` (10/11/2026, support du firmware 23.5.0), en cours de vérification.

## D9 — Musiques de fond retirées du paquet (25/11/2026)
Les 4 MP3 du menu (24,7 Mo) sont sortis de `romfs/` vers `assets/musique_optionnelle/` (option 3 de D5). `AudioManager::loadTrack` gère leur absence. HomeNX+.nro passe de 40,9 à 17 Mo. On peut les remettre en les replaçant dans `romfs/sounds/wiiu/music/`.

## D10 — Claude : API directe, Haiku 5.5 par défaut (25/11/2026)
- Pas de SDK officiel en C++ pour la Switch : requêtes HTTP directes sur `https://api.anthropic.com/v1/messages`, en-têtes `x-api-key` et `anthropic-version: 2026-10-21`, streaming SSE.
- Modèle par défaut **claude-haiku-5-5** (0,10 / 0,50 $ par Mtok ; au-delà de 100K de prompt : 0,50 / 2,50 $), option **claude-opus-5-5** (4 / 20 $), comme le demande le cahier des charges.
- Pas de paramètre `thinking` (adaptatif par défaut sur ces modèles ; `disabled` y est refusé) ; `output_config.effort = low` par défaut pour le chat. Les blocs `thinking` reçus sont renvoyés tels quels dans l'historique.
- Coût calculé à chaque réponse, registre `homenxplus/claude/usage-AAAA-MM.csv`, plafond bloquant en euros (`monthly_cap_eur`, 5 € par défaut). Un modèle inconnu est compté au tarif Opus, pour ne jamais sous-estimer.
- Certificats : `assets/cacert.pem` (Mozilla, publié par curl.se, SHA-256 vérifié) copié dans `config/homenxplus/cacert.pem`.
- La clé (`config/homenxplus/api_key.txt`) n'est jamais affichée ni journalisée ; curl est en mode non verbeux.

## D11 — Onglet « HomeNX+ » dans les réglages du menu (25/11/2026)
- Un 13e onglet (index 12, `HomeNXPlusTab.cpp`) rend visibles les fonctions déjà prêtes :
  - Claude : clé présente ou non, dépense du mois, bouton « Ouvrir ClaudeNX » ;
  - Scènes : sélecteur qui applique la scène ;
  - Batterie : niveau, et alerte AutoRCM sous 15 % ;
  - Rangement : import CardsNX vers `homenxplus/layout.json` ;
  - À propos.
- Les scènes sont appliquées par un client sys-clk écrit pour HomeNX+ (`src/switch/actions.cpp`), d'après le protocole public de sys-clk : service `sys:clk`, commande 8 `SetOverride {module, hz}`, 0 Hz = forçage retiré. La luminosité passe par `lbl`. Fizeau n'est pas encore piloté.
- Ouverture d'un `.nro` depuis les réglages : nouveau rappel `SettingsScreen::onLaunchNro`, branché sur `leaveThen` puis `launchNro`.
- Cache xmake : après une modification de `xmake.lua`, d'anciennes informations de paquets ont réintroduit `-ldeko3dd`. Remède : supprimer les dossiers `.xmake/**/cache` et `build/.buildcache`, puis `build.ps1 -Force`.

## D12 — Bibliothèques debug vides et zlib direct (25/11/2026)
- Le doublon `libdeko3dd` revenait après chaque changement de configuration. Cause : les fichiers `.pc` de SDL2 ajoutent `deko3d` et `deko3dd` via le modèle `switch-pkg`, que xmake prend dans le dépôt d'origine, donc sans notre filtre.
- Solution durable : `lib/hnp_stubs/` contient `libdeko3dd.a`, `libnxd.a` et `libdrm_nouveaud.a` **vides**, cherchées en premier (`add_linkdirs` global). `-ldeko3dd` ne peut plus créer de doublon, quel que soit le cache.
- Le démon relie `z` directement (`add_syslinks("z", "nx")`), car le paquet zlib pouvait être ignoré après un vidage du cache.
- Les deux variantes (test `.nro` et vrai menu) compilent ensemble sans vider le cache.

## D13 — Mode sans échec et français complet (25/11/2026)
- Compteur `config/<menu>/hnp_starts` : +1 à chaque démarrage, remis à 0 après 30 s ou à une sortie propre. À partir de 3, le menu coupe pour ce démarrage les effets de verre, la musique et SteamGridDB. L'état est affiché dans l'onglet HomeNX+.
- `romfs/i18n/fr-FR.json` : 439 textes ajoutés (`outils/i18n/fr_ajouts.json` + `fusionner_fr.py`), 0 manquant. « Éteindre » rappelle le risque AutoRCM.
- Tâche planifiée Windows « HomeNX+ session de nuit » (tous les jours à 2 h, session ouverte seulement) ; `nuit.ps1` s'arrête si Claude Code n'est pas connecté.

## D14 — Apparence Cartes et bouton Style (25/11/2026)
- Le menu quotidien est en apparence Cartes : les ajouts HomeNX+ y sont maintenant (`projects/cards/src/CardsHnp.cpp`, compilé seulement avec `HNP_PLUS`, donc pas dans CardsNX seul).
- Changer de style = écrire `frontend.txt` puis quitter le menu : le démon le relance avec le nouveau style (vrai menu), le `.nro` se relance lui-même (`envSetNextLoad`). Refusé si un jeu est ouvert : le démon ne relance le menu que s'il n'y a rien à l'écran.
- Le `.nro` de test lit les fichiers de CardsNX installés (`switch/SwitchU/cards/`, lecture seule) et écrit ses réglages dans `config/homenxplus/cards`.
- `ReleaseUpdater` (mise à jour HomeNX depuis GitHub) neutralisé sous `HNP_PLUS` : HomeNX+ se met à jour seulement depuis le PC.

## D15 — Netflix, Disney+, Prime Video, Spotify : par le PC (25/11/2026)
- Aucune appli officielle sur Switch, et le navigateur de la console n'a pas le DRM Widevine : impossible en direct, et contourner un DRM est exclu.
- Choix : le PC ouvre le site (`pc_agent`, `POST /ouvrir`, **liste fermée** de 4 adresses, jeton, réseau local), puis HomeNX+ lance Moonlight. Réveil Wake-on-LAN et attente de 60 s au plus.
- Spotify : télécommande Spotify Connect déjà présente dans CardsNX (vrai menu) ; le son sur la Switch passe aussi par Moonlight.
