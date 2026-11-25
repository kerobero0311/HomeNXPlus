# Analyse de HomeNX+ et de la Switch (25/11/2026)

Base de l'analyse : le code de HomeNX+ 0.9.1, l'inventaire de la carte SD (47 applis `.nro`,
15 overlays, 6 sysmodules au démarrage, 13,2 Go libres) et tes tests sur la console.

## 1. Bilan par critère

| Critère | Note | Ce qui va | Ce qui manque |
|---|---|---|---|
| Simplicité | 6/10 | Tout part des trois points ; menus courts ; A choisir / B retour partout | Trois portes vers les mêmes choses (tuile HomeNX+, onglet HomeNX+, bouton +) ; certaines fonctions demandent le PC sans le dire avant |
| Cohérence | 5/10 | Même style visuel que CardsNX ; textes en français | Les deux apparences n'ont pas les mêmes fonctions (Wii U : onglet seulement ; Cartes : panneau, fiche, Médias) ; quelques messages sans accents (lien PC, scènes) |
| Légèreté | 8/10 | Le `.nro` relit les fichiers de CardsNX au lieu de les copier ; 18,8 Mo ; aucune tâche de fond | `tico.nro` en double (2 × 40,8 Mo) ; HomeNX recopie 4 applis de `SwitchU/bin` dans `switch/` |
| Stabilité | 7/10 | Mode sans échec ; démarrage marqué réussi dès l'écran prêt ; erreurs de démarrage notées sur la carte | Le plantage du démarrage a montré qu'il faut tester chaque version sur la console avant diffusion |
| Fonctions | 7/10 | Claude, fiche de jeu, scènes, lien PC, Médias, pages intelligentes | Dossiers dans la grille, widgets, éditeur de scènes, mises à jour de HomeNX+ |
| Sécurité | 9/10 | Clé API jamais affichée ; capture envoyée seulement après « Oui » ; PC sur réseau local avec jeton ; liste fermée des sites | Aucune limite de taille pour les questions envoyées à Claude |
| Prêt à diffuser | 4/10 | Code propre, licences et crédits en ordre | Pas encore installé comme vrai menu HOME ; pas de version en anglais ; pas de page de téléchargement |

## 2. Fait dans cette passe (0.9.1)
- **Page Médias** : Netflix, Disney+, Prime Video, Spotify (par le PC et Moonlight) et Discord y sont des cartes ; Moonlight, Switchfin, StremNX, TsVitch, NXMP et wiliwili y sont rangées d'office quand elles sont sur la carte. La tuile « Films » disparaît : tout est sur une seule page, comme les jeux.
- **La scène d'un jeu s'applique vraiment** à son lancement, puis s'arrête au retour au menu (avant : enregistrée mais jamais appliquée).
- **Garde batterie** : alerte à 15 % puis 8 % (notification + message), avec le rappel AutoRCM.
- **Message « en cours » qui reste affiché** tant que Claude ou le PC travaille (avant : 4 secondes puis plus rien).
- **Checklist cochable dans le menu** (+ sur un jeu > Fiche HomeNX+ > Cocher la checklist), plus seulement dans l'overlay.
- **Accents** rétablis dans tout le panneau HomeNX+ et l'onglet HomeNX+.

## 3. À faire avant de diffuser (priorité 1)
1. **Installer comme vrai menu HOME** (phase 12) et faire les 50 lancements de jeu sans plantage.
2. **Mêmes fonctions dans les deux apparences** : panneau HomeNX+ et cartes Médias aussi dans l'apparence Wii U.
3. **Une seule porte d'entrée claire** : la tuile HomeNX+ ouvre tout ; l'onglet des réglages garde seulement les réglages (style, clé, PC).
4. **Mises à jour de HomeNX+** depuis GitHub (bouton « Mettre à jour » dans le panneau, vérification SHA-256), puisque celles de HomeNX sont coupées.
5. **Version anglaise** des textes HomeNX+ (fichier de langue, comme CardsNX), pour la diffusion.
6. **Premier lancement guidé** : 3 écrans (style, clé Claude facultative, lien PC facultatif).
7. **Messages restants à accentuer** (lien PC, scènes, batterie) et relecture de tous les textes.

## 4. Améliorations (priorité 2)
- **Dossiers dans la grille des Cartes** (la logique est prête et testée : `hnp/layout`).
- **Widgets sur l'accueil** : dernier jeu, temps de jeu du jour, scène active, messages du PC.
- **Éditeur de scènes** dans le menu (aujourd'hui : fichier `scenes.json`).
- **Claude : contexte au choix** (cases : jeu en cours, notes, dernière capture) et rangement assisté.
- **Indicateur PC** dans la barre du haut (allumé / éteint) et réveil automatique avant Moonlight.
- **Fiche de jeu** : temps de jeu, dernières sessions, captures du jeu.
- **Thème selon l'heure** (jour / nuit) et taille des cartes réglable.

## 5. Applis : ce qui est là, ce qui manque

**Déjà sur la carte (bien couvert)** : rétro et Wii/GC (tico), Wii U (cemu-nx), PC (Moonlight),
films perso (Switchfin pour Jellyfin), Stremio (StremNX), TV en direct (TsVitch), sauvegardes (JKSV),
fichiers (NX-Shell, sphaira), temps de jeu (NX-Activity-Log, GameTime), thèmes (NXThemes, Themezer),
mods (SimpleModManager), manettes (sys-con, MissionControl), réglages (Ultrahand, sys-clk, Fizeau, FPSLocker, ReverseNX).

**Propositions (vérifiées sur GitHub le 25/11/2026, versions publiées officiellement)** :

| # | Appli | Version | Taille | À quoi ça sert | Avis |
|---|---|---|---|---|---|
| 1 | SwitchWave (averne) | v1.1.2, 16/09/2026 | 13 Mo | Lecteur vidéo avec décodage matériel (jusqu'à la 4K) : films du PC ou d'un disque réseau (SMB, NFS, SFTP, HTTP) ou d'un disque USB, sans Moonlight | **Fortement conseillé** (meilleur que NXMP, plus léger) |
| 2 | GMCA (ex-pleNx, thcolin) | v1.5.0, 19/11/2026 | 29 Mo | Plex, Jellyfin, Emby et Stremio dans une seule appli | **Conseillé** : peut remplacer Switchfin + StremNX (62 Mo) après essai |
| 3 | Switch-NewPipe (mirusu400) | v0.0.5, 19/08/2026 | 31 Mo | YouTube sans compte ni pub | Conseillé, mais jeune (version 0.0.x) |
| 4 | NXMilk (proconsule) | v0.4.1, 21/08/2026 | 19 Mo | Musique (SD, USB, réseau) avec visualisations | Utile si tu as tes MP3 / FLAC |
| 5 | BookReader-NX (ex-WookReader) | aucune version publiée | — | BD, mangas, PDF, EPUB | À attendre |
| 6 | chiaki-ng (PlayStation à distance) | v1.10.1 sans version Switch | — | Jouer à sa PS4 / PS5 | Seulement si tu as une PlayStation |
| — | NXMP | v0.9.3, 12/2025 | 43 Mo | Lecteur vidéo | Doublon de SwitchWave, plus lourd |
| — | SwitchMX | v1.0.0, 18/11/2026 | 49 Mo | Lecteur tout-en-un | Trop récent et peu éprouvé pour l'instant |

Bilan place : ajouter 1 + 2 + 3 + 4 = 92 Mo ; retirer Switchfin + StremNX (si GMCA convient) et le doublon de tico = 103 Mo. **Résultat : plus de fonctions et 11 Mo gagnés.**
Toutes ces applis se rangeront d'office dans la page Médias de HomeNX+.

**Impossibles en direct (pas d'appli Switch, protection anti-copie)** : Netflix, Disney+, Prime Video.
Ils passent par le PC (carte Médias > un appui : réveil du PC, site ouvert, Moonlight).
Spotify : télécommande (Spotify Connect) ou par le PC.

**Place à récupérer (avec ton accord)** : `switch/tico.nro` est un doublon de `switch/tico/tico.nro` (40,8 Mo).

## 6. Le mix des projets : ce que HomeNX+ reprend de chacun (relevé du 25/11/2026)

| Projet | Ce qu'il a de meilleur | Dans HomeNX+ |
|---|---|---|
| HomeNX (ad3333m, GPL-3.0) | Apparences Wii U et Cartes, Discord, écran de verrouillage, succès, SteamGridDB, NXToggle | **Base de tout** (à jour : aucun nouveau commit depuis notre départ) |
| SwitchU 1.4.2 (PoloNX, GPL-3.0) | Dossiers sur plusieurs cases, grille portrait, affiches SteamGridDB, réglage TV/portable du processeur | Dossiers et grille : déjà dans l'apparence Wii U. Correctifs TV/portable : sans objet (notre démon n'a pas ce code) |
| SwitchU de tomvita (GPL-3.0) | Filtre par nom et clavier USB, tri Récents, HOME qui bascule entre le jeu et le menu | **Recherche ajoutée** : touche Y, tuile Rechercher ou une lettre tapée au clavier USB. Tri Récents : déjà là. Bascule HOME : à faire |
| uLaunch (GPL-2.0 seule) | Applis `.nro` en cartes sans forwarder, thèmes | `.nro` en cartes : déjà là (page Homebrew). Thèmes : couleurs, verre, fond. Aucun code copié (licence incompatible) |
| sLaunch (GPL-3.0+) | Plusieurs modes d'affichage, widgets, packs d'icônes | Deux apparences et le bouton Style. Widgets : apparence Wii U seulement. Mode liste compacte : à faire |
| CardsNX (dans HomeNX) | Cartes jaquettes, page Médias, verre, lecteur de musique, Spotify | Repris, avec le panneau HomeNX+ et une page Médias étendue |
| ChatNX, pocket-deepchat | IA sur Switch, envoi de captures | Claude en direct, sans passerelle |
| **HomeNX+ (ajouts propres)** | — | Claude, scènes, overlay en jeu, lien PC, Netflix et co par le PC, fiche de jeu, recherche, garde batterie, mode sans échec |

**Pour l'interface ultime, il reste** (dans l'ordre) :
1. ~~Médias dans l'apparence Wii U~~ : fait (0.9.3, onglet HomeNX+). Reste : le panneau complet (fiche de jeu, recherche) en Wii U.
2. ~~HOME qui ramène au jeu en pause~~ : déjà fait par CardsNX (HOME ferme les fenêtres puis reprend le jeu ouvert).
3. Un mode « liste compacte » dans les Cartes (sLaunch) : à faire, à vérifier sur la console écran par écran.
4. Widgets dans les Cartes : la barre du haut est pleine ; à placer dans le panneau du bas (dernier jeu, temps du jour, scène active).
5. Les dossiers dans les Cartes : en partie couverts par les pages perso (pages intelligentes et rangement par Claude).
