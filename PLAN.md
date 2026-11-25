# PLAN — HomeNX+

Statuts : `[ ]` à faire · `[~]` en cours · `[x]` fait · `[T]` à tester sur la console · `[!]` bloqué
Étiquettes : `[nuit-ok]` faisable sans l'utilisateur · `[jour]` demande la console ou une décision

## Session 1 — démarrage
- [x] Lire `CONTEXTE_SWITCH.md` `[nuit-ok]`
- [x] Origine de HomeNX : ad3333m/homenx build 199, GPL-3.0 (DECISIONS D1) `[nuit-ok]`
- [x] Veille de la section 5 → `VEILLE.md` `[nuit-ok]`
- [x] Base du fork : ad3333m/homenx, correctifs repris de tomvita (DECISIONS D6) `[nuit-ok]`
- [x] PLAN, JOURNAL, TESTS_CONSOLE, DECISIONS, VEILLE `[nuit-ok]`
- [~] Questions : nom, style, budget, IP, dock `[jour]` (réponses par défaut dans D5)
- [x] Client Discord intégré : conservé (D7) `[jour]`

## Session de nuit — mise en route
- [x] `NUIT.md` et `nuit.ps1` (corrigé, testé à vide) `[nuit-ok]`
- [x] Claude Code en ligne de commande installé sur Windows `[jour]`
- [ ] Première connexion de `claude` en ligne de commande (compte Claude) `[jour]`
- [x] Tâche planifiée Windows « HomeNX+ session de nuit » à 2 h `[jour]`

## Sécurité de la console
- [x] Guide `docs/SAUVEGARDE_NAND.md` + `outils/vider_sauvegarde_nand.ps1` `[nuit-ok]`
- [ ] Sauvegarde NAND sysMMC faite par l'utilisateur `[jour]`

## Phase 0 — environnement et chaîne de test
- [x] devkitPro (dans MSYS2) : devkitA64, libnx 4.12, switch-tools + portlibs (sdl2, mixer, ttf, curl, mbedtls, zlib, freetype, png, jpeg, webp, mesa, deko3d…) `[jour]`
- [x] xmake 3.1.2 (portable) + MinGW hôte pour les données eSpeak `[jour]`
- [x] Clone ad3333m/homenx + sous-modules, remote `upstream`, branche `main` `[jour]`
- [x] Compilation `.nro` telle quelle : HomeNX, CardsNX, NXToggle, Homebrew Manager, Loader (3 correctifs Windows, D8) `[nuit-ok]`
- [x] Compilation complète daemon + menu sans installation (Atmosphere-libs à jour : switchu-daemon.nsp 674 Ko, switchu-menu.nsp 10,2 Mo) `[nuit-ok]`
- [x] `outils/env.ps1`, `build.ps1` (celui d'origine, branché sur env.ps1) `[nuit-ok]`
- [x] `deploy.ps1` (FTP, liste blanche, .tmp + renommage, VERSIONS.txt, `-Simulation`) `[nuit-ok]`
- [x] `logs.ps1` (lecture seule) `[nuit-ok]`
- [x] `test.ps1` : 3 tests d'origine OK sous Windows, 2 tests POSIX sautés `[nuit-ok]`
- [ ] Environnement Python `.venv` pour pc_agent et les tests `[nuit-ok]`
- [x] `outils/installer_sur_sd.ps1` (carte dans le PC ou UMS ; liste blanche, SHA-256) `[nuit-ok]`
- [ ] Premier essai FTP réel (deploy + logs) `[jour]`

## Phase 1 — stabilité
- [T] Renommage HomeNX+ (nom, auteur, CREDITS.md) ; le .nro range ses données dans `config/homenxplus/menu` et `switch/HomeNXPlus/data` (macros HNP_*) ; copié sur la SD le 24/11 `[nuit-ok]`
- [ ] Client Discord intégré conservé (D7) : vérifier qu'il compile et ne journalise jamais le jeton `[nuit-ok]`
- [ ] Client Discord : test de connexion et de messages sur la console `[jour]`
- [ ] Comparer avec tomvita/SwitchU et reprendre les correctifs manquants (clavier USB, `avm`, retour après le logo…) `[nuit-ok]`
- [ ] Journal mémoire du menu (`homenxplus/logs/`) `[nuit-ok]`
- [x] Allègement : HomeNX+.nro 40,9 → 17 Mo (musiques retirées du romfs, D9) `[nuit-ok]`
- [ ] Chargement paresseux des icônes et bannières, cache disque borné, libération des textures hors écran `[nuit-ok]`
- [T] Mode sans échec après 3 plantages de suite (verre, musique, SteamGridDB coupés ; état dans l'onglet HomeNX+) `[nuit-ok]`
- [ ] Mesure de la mémoire avec les 6 sysmodules `[jour]`
- [ ] 50 cycles lancer un jeu / revenir au menu `[jour]`

## Phase 2 — français à 100 % et réglages
- [x] Traductions : 439 textes ajoutés, 0 manquant dans `fr-FR.json` `[nuit-ok]`
- [ ] Réglages non terminés du projet d'origine `[nuit-ok]`
- [ ] Icônes propres et légères `[nuit-ok]`

## Phase 3 — rangement
- [x] Modèle `layout.json` (dossiers, sous-dossiers sur 1 niveau, ordre, styles) + tests PC (`hnp_layout`) `[nuit-ok]`
- [x] Import du rangement CardsNX (orderGames, hiddenCards), logique testée `[nuit-ok]`
- [T] Dossiers intelligents : logique testée + aperçu dans l'onglet HomeNX+ (Récents, Les plus joués, Jamais lancés, depuis ns et pdm) `[nuit-ok]`
- [x] Tris (7 modes), mémorisés par dossier (logique testée) `[nuit-ok]`
- [ ] Filtres et recherche (clavier système + clavier USB) `[nuit-ok]`
- [x] Favoris, épinglage, masquage, annulation sur 20 niveaux (logique testée) `[nuit-ok]`
- [x] Sauvegarde .tmp + 5 versions (testée) ; export/import PC à faire avec pc_agent `[nuit-ok]`
- [ ] Glisser tactile et sélection multiple `[nuit-ok]`
- [ ] Test sur la console `[jour]`

## Visibilité (onglet HomeNX+ des réglages)
- [T] Onglet HomeNX+ : Claude (clé, dépense, ouvrir ClaudeNX), scènes, batterie, import du rangement, à propos `[nuit-ok]`

## Phase 4 — styles d'interface
- [ ] Cartes (amélioré), Grille compacte, Liste détaillée, Étagère, Minimal, Style Switch `[nuit-ok]`
- [ ] Style par dossier, taille des icônes, thème jour/nuit, accent, fonds, animations, sons `[nuit-ok]`
- [ ] Budget mémoire par style `[nuit-ok]`
- [ ] Test sur la console `[jour]`

## Phase 5 — raccourcis et dock
- [ ] Cartes `.nro` sans forwarder (étude d'uLaunch, sans copier de code) `[nuit-ok]`
- [ ] Dock en bas de l'écran (`shortcuts.json`) `[nuit-ok]`
- [ ] Raccourcis tico : vérifier si une ROM peut être passée en argument `[nuit-ok]`
- [ ] Raccourcis web : vérifier l'applet web en homebrew `[jour]`
- [ ] Raccourcis système (Linux / Hekate / Atmosphère officiel, Album, réglages, test des manettes) + confirmation `[nuit-ok]`
- [ ] Combinaisons de touches configurables `[nuit-ok]`
- [ ] Test sur la console `[jour]`

## Phase 6 — widgets
- [ ] Widgets C++ configurés en JSON : batterie (alerte AutoRCM < 15 %), température, SD (< 5 Go), heure, temps du jour, dernier jeu, messages, scène `[nuit-ok]`
- [ ] Déplacement et désactivation `[nuit-ok]`

## Phase 7 — fiche de jeu
- [ ] Notes (swkbd), checklist, temps, sessions, captures, Claude, scène, dossier, favori, bannière `[nuit-ok]`
- [ ] Test sur la console `[jour]`

## Phase 8 — Claude
- [x] Modèles et format vérifiés (skill claude-api : `anthropic-version: 2026-10-21`, Haiku 5.5 par défaut, Opus 5.5 en option) `[nuit-ok]`
- [x] `libclaude` : HTTPS libcurl + mbedTLS, SSE, erreurs en français + tests PC (`hnp_claude`) `[nuit-ok]`
- [x] Coût, plafond mensuel bloquant (`monthly_cap_eur`), limite de tokens (`max_tokens`) `[nuit-ok]`
- [T] `ClaudeNX.nro` autonome v0.1 (console texte, clavier système, A/Y/X/B/+, 1,3 Mo) `[nuit-ok]`
- [~] Historique (`homenxplus/chats/`, fait) et recherche (à faire) `[nuit-ok]`
- [~] Vision (ClaudeNX ZR), checklist par Claude et **traduction de capture** (enregistrée dans les notes) faites ; contexte à cocher, rangement assisté et mode agent à faire `[nuit-ok]`
- [ ] Mode agent : liste blanche fermée + confirmation `[nuit-ok]`
- [ ] Clé API sur console.anthropic.com avec limite de dépenses `[jour]`
- [ ] Test réel `[jour]`

## Phase 9 — scènes et garde batterie
- [x] `scenes.json` : modèle et validation (fréquences Erista officielles), testés `[nuit-ok]`
- [T] Application réelle des scènes : IPC sys-clk + luminosité + Fizeau (service fizeau, SetIsActive) `[nuit-ok]`
- [x] Scène par jeu, garde batterie 15 % / 8 % avec hystérésis (logique testée) `[nuit-ok]`
- [ ] Test sur la console `[jour]`

## Phase 10 — overlay
- [ ] `HomeNXPlus.ovl` (libultrahand) : notes, checklist, traduction, scènes, mini-état `[nuit-ok]`
- [ ] Test en jeu `[jour]`

## Phase 11 — PC et Linux
- [x] `pc_agent` : serveur local (réseau privé uniquement) + jeton, boîte aux lettres, dépôt de fichiers protégé, résumé de la semaine, MAC pour le réveil ; 12 tests `[nuit-ok]`
- [T] HomeNX+ : section PC (réveil Wake-on-LAN, envoyer et lire des messages) `[nuit-ok]`
- [T] Envoi des captures de l'Album vers le pc_agent (30 dernières non envoyées) `[nuit-ok]`
- [T] Copie des saves du dernier jeu vers le pc_agent (lecture seule, fsdevMountSaveData) `[nuit-ok]`
- [ ] Lancement de Moonlight après le réveil, démarrage auto de pc_agent avec Windows (avec accord) `[jour]`
- [ ] SwitchPresence-Rewritten : compatibilité firmware 23, puis installation avec accord, interrupteur dans HomeNX+ `[jour]`
- [x] Linux : `secours_menu_home.sh` (testé, copié dans switchroot/outils/homenxplus) `[nuit-ok]`
- [x] Linux : `boite_aux_lettres.sh`, `rapport_sante.sh`, `installer.sh` (icônes KDE), copiés sur la SD `[nuit-ok]`
- [x] `secours_menu_home.ps1` (testé sur une fausse carte) `[nuit-ok]`
- [ ] Tests PC ↔ Switch et Kubuntu `[jour]`

## Phase 12 — installation comme menu HOME (EN PRÉSENCE DE L'UTILISATEUR)
- [ ] Tout validé en .nro, phase 1 comprise `[jour]`
- [ ] Sauvegarde du HomeNX actuel + SHA-256 `[jour]`
- [ ] Test du plan de secours `[jour]`
- [ ] Installation atomique daemon + menu `[jour]`
- [ ] Checklist de premier démarrage `[jour]`

## Phase 13 — sysmodule léger (optionnel)
- [ ] Santé toutes les 10 min + tableau de bord web local, désactivé par défaut `[nuit-ok]`

## Qualité
- [x] README.md (français), CHANGELOG.md, CREDITS.md `[nuit-ok]`
