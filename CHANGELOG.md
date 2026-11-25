# Changelog — HomeNX+

## 0.1.0 (en cours, 24/11/2026)
- Fork de HomeNX (ad3333m/homenx, build 213) sous GPL-3.0, avec crédits.
- Compilation sous Windows natif : libnx 4.12, bibliothèques debug ignorées, ordre de liaison webp, Atmosphere-libs à jour.
- Scripts : `outils/env.ps1`, `test.ps1`, `deploy.ps1` (FTP sécurisé), `logs.ps1`, `nuit.ps1`.
- Secours : `secours_menu_home.ps1` (Windows) et `linux/secours_menu_home.sh` (Kubuntu).
- Le `.nro` de test range ses données à part (`config/homenxplus/`, `switch/HomeNXPlus/`) et ne touche pas au HomeNX installé.

## 0.2.0 (25/11/2026)
- **ClaudeNX.nro** : Claude en application autonome. Interface texte, clavier système, réponses en direct, coût affiché, plafond mensuel, historique sauvegardé. Touches : A question, Y nouvelle conversation, X changer de modèle, B arrêter, + quitter.
- **libclaude** : client de l'API Messages en streaming, partagé avec HomeNX+, avec tests sur PC.
- **HomeNX+.nro allégé** : 17 Mo au lieu de 40,9 Mo (musiques de fond retirées).
- Guide de sauvegarde NAND (`docs/SAUVEGARDE_NAND.md`) et script `outils/vider_sauvegarde_nand.ps1`.
- `outils/installer_sur_sd.ps1` : installation sur la carte sans risque pour le menu HOME.

## 0.3.0 (25/11/2026)
- ClaudeNX : **ZR** joint la dernière capture d'écran de l'Album à la question suivante (vision).
- Rangement (`hnp/layout`) : dossiers et sous-dossiers, favoris, épinglage, masquage, 20 annulations, 7 tris par dossier, recherche sans accents, dossiers intelligents, import du rangement CardsNX, sauvegarde avec 5 versions. Logique prête et testée ; l'interface viendra ensuite.
- Scènes (`hnp/scenes`) : Nuit, Batterie, Perf max et scènes perso (fréquences vérifiées), scène par jeu ; garde batterie 15 % / 8 % avec rappel AutoRCM.
- Linux : `rapport_sante.sh`, `boite_aux_lettres.sh`, `installer.sh` (icônes KDE).

## 0.3.1 (25/11/2026)
- Nouvel onglet **HomeNX+** dans les réglages du menu : Claude (clé, dépense du mois, ouvrir ClaudeNX), **scènes appliquées pour de vrai** (fréquences via sys-clk et luminosité), niveau de batterie avec alerte AutoRCM, import du rangement CardsNX, à propos.

## 0.4.0 (25/11/2026)
- **Menu 100 % en français** : 439 textes traduits (réglages, Social, Mii, Discord, thème auto, dossiers…).
- **Mode sans échec** : après 3 démarrages ratés de suite, le menu démarre allégé (sans verre, musique ni images SteamGridDB).
- Compilation fiable des deux versions (test et vrai menu) sans vider le cache.
- Sessions automatiques de nuit planifiées à 2 h (actives une fois Claude Code connecté sur le PC).

## 0.5.0 (25/11/2026)
- **Compagnon PC (pc_agent)** : petit serveur Python sur le PC, réseau privé seulement et protégé par jeton. Il gère la boîte aux lettres Switch ↔ PC, le dépôt de fichiers (saves, captures) dans `Documents\Switch_Sauvegardes\<date>\` et le résumé de la semaine. Lancement : `pc_agent\lancer_pc_agent.ps1`.
- Onglet HomeNX+, section **PC** : état du lien, réveil du PC (Wake-on-LAN), envoyer et lire des messages.
- `installer_sur_sd.ps1` écrit `config/homenxplus/pc_agent.ini` automatiquement.
- Tests : 9 suites OK (C++ et Python).

## 0.6.0 (25/11/2026)
- **Overlay HomeNX+ en jeu** (`switch/.overlays/HomeNXPlus.ovl`, ouvert par Ultrahand) : batterie, scènes à appliquer, notes et checklist du jeu en cours à cocher.
- **Envoi des captures au PC** depuis HomeNX+ (onglet HomeNX+ > PC).
- Notes et checklists par jeu (fichiers `homenxplus/notes` et `checklists`), checklist créée à partir d'un texte de Claude.
- 10 suites de tests OK.

## 0.7.0 (25/11/2026)
- Onglet HomeNX+ : section **Dernier jeu**, avec les notes à modifier au clavier et une **checklist de complétion créée par Claude**, à cocher dans l'overlay.
- Aperçu des **dossiers intelligents** : jeux récents, les plus joués (en heures), jamais lancés.

## 0.8.0 (25/11/2026)
- **Traduction d'une capture par Claude** : le texte de ta dernière capture est traduit en français, puis ajouté aux notes du jeu (visibles dans l'overlay).
- **Copie des saves du dernier jeu sur le PC**, en lecture seule (pc_agent).
- **Fizeau piloté par les scènes** : la scène Nuit allume le filtre ; il s'éteint quand on quitte la scène, seulement si c'est HomeNX+ qui l'avait allumé.

## 0.9.0 (25/11/2026)
- **Apparence Cartes (CardsNX) dans HomeNX+**, avec ses ajouts. Le `.nro` de test s'ouvre en Cartes par défaut (ton rangement CardsNX recopié, l'original n'est que lu).
- **Bouton Style** : trois points > Style (Cartes → Wii U) et onglet HomeNX+ du look Wii U (Wii U → Cartes). Le menu redémarre tout seul en 2 s.
- **Panneau HomeNX+** (trois points > HomeNX+, et Réglages > HomeNX+) : Claude (question, « que faire ici ? » sur la dernière capture, traduction), fiche du jeu choisi, scènes, PC, pages intelligentes (Récents, Plus joués, Jamais lancés), batterie et santé, à propos.
- **Fiche de jeu** (+ sur un jeu > Fiche HomeNX+) : notes, checklist (création par Claude), question à Claude sur ce jeu, scène du jeu, copie des saves sur le PC.
- **Films, séries et musique** (trois points > Films) : Netflix, Disney+, Prime Video, Spotify. Un appui réveille le PC si besoin, `pc_agent` y ouvre le site (liste fermée), puis Moonlight démarre. Télécommande Spotify (Spotify Connect) dans le vrai menu.
- `pc_agent` 0.2.0 : `POST /ouvrir` limité à 4 adresses, testé.
- Mise à jour automatique de HomeNX désactivée dans HomeNX+ (elle remplacerait le fork).
- Réglages CardsNX du `.nro` de test dans `config/homenxplus/cards` (macro `HNP_CFG_CARDS`).

## 0.9.1 (25/11/2026)
- Correction : le `.nro` s'arrêtait au démarrage (fil de 30 s impossible à créer sur la console) ; démarrage marqué réussi quand l'écran est prêt ; erreurs de démarrage notées dans `config/homenxplus/cards/start.log`.
- Page Médias : cartes Netflix, Disney+, Prime Video, Spotify (par le PC) et Discord ; Moonlight, Switchfin, StremNX, TsVitch, NXMP, wiliwili rangées d'office. Tuile « Films » retirée.
- La scène d'un jeu s'applique à son lancement et s'arrête au retour au menu.
- Garde batterie 15 % / 8 % dans l'apparence Cartes.
- Message « en cours » visible pendant tout le travail de Claude ou du PC.
- Checklist cochable dans le menu ; accents rétablis dans les textes HomeNX+.
- `ANALYSE.md` : analyse complète et liste priorisée avant diffusion.

## 0.9.2 (25/11/2026)
- Recherche par nom dans les Cartes : touche Y, tuile Rechercher, ou une lettre tapée au clavier USB (idée de tomvita/SwitchU). Résultats de toutes les pages, sans tenir compte des accents.
- Clic du stick gauche : ouvre le panneau HomeNX+.
- Page Médias : SwitchWave, GMCA, NXMilk, Switch-NewPipe reconnues.
- ANALYSE.md : tableau du mix des projets (HomeNX, SwitchU, tomvita, uLaunch, sLaunch, CardsNX).

## 0.9.3 (25/11/2026)
- Rangement assisté par Claude : il propose des pages par genre (Aventure, Course...), tu acceptes ou non ; logique testée (`hnp/assist`, test `hnp_assist`).
- « Jeux du PC » : réveille le PC si besoin puis lance Moonlight (panneau HomeNX+ et carte de la page Médias).
- Apparence Wii U : section Médias (Netflix, Disney+, Prime Video, Spotify par le PC, puis Moonlight) dans l'onglet HomeNX+.

## 0.9.3 (suite, 25/11/2026)
- Dossier hors ligne (accueil, guide + PDF, trailer, Ludothèque sans compte, feuille de tournage) sur le Bureau, sur la carte (homenxplus/docs) et en un lien.
- Tournage : Switch Frame Tap (menu HOME filmé en MP4 sur le PC, par câble USB avec le pilote WinUSB de Zadig), filmer_ecran.sh sous Linux, recuperer_tournage.ps1.
- Paquet de diffusion : outils\creer_release.ps1 (zip SD + PC + guide, SHA-256, garde-fou contre les données personnelles).
- Tests et documents : adresse IP locale remplacée par une adresse d'exemple.
- Tout en une passe : outils\tout_en_une_passe.ps1 (+ raccourci « Switch - Tout mettre a jour » sur le Bureau) : santé de la console, compilation si besoin, installation, documents, journaux, captures et rushes, bilan.

## 0.9.4 (25/11/2026)
- Mise à jour de HomeNX+ depuis l'appli (panneau > Mettre à jour HomeNX+, depuis les versions GitHub, .nro vérifié).
- Assistant de premier lancement (style, Claude, PC, raccourcis).
- Statistiques de jeu ; captures du jeu dans sa fiche ; Claude : choisir ce qu'on joint (jeu, notes, capture) ; thème nuit automatique.
- 16 services par le PC ; boutique d'applis (35 applis) et son installeur ; publication GitHub et site.
- FEUILLE_DE_ROUTE.md (état des 50 choix), promo/textes_diffusion.md.
