# TESTS CONSOLE â€” HomeNX+

Ã€ faire sur la Switch. Coche quand c'est validÃ© et note ce qui s'est passÃ©.

## 1. HomeNX+ en application de test (dÃ©jÃ  sur la carte depuis le 24/11)
Le vrai menu HOME n'est pas modifiÃ© : rien ne peut casser le dÃ©marrage.
- [ ] Ouvrir le menu homebrew (album, ou carte Homebrew de HomeNX), lancer **HomeNX+**
- [ ] L'interface s'affiche (apparence HomeNX / Wii U) et tes jeux apparaissent
- [ ] Lancer un jeu depuis HomeNX+, puis revenir avec HOME
- [ ] Changer un rÃ©glage dans HomeNX+, quitter, et vÃ©rifier que **ton HomeNX normal n'a pas changÃ©**
- [ ] Si Ã§a plante : noter l'Ã©cran, puis carte dans l'Ally (je lis `homenxplus/logs/` et `atmosphere/crash_reports/`)

## 1 A. NouveautÃ©s 0.9 : apparence Cartes, Style, panneau HomeNX+ (dans HomeNX+.nro)
- [ ] Ouvrir HomeNX+ depuis le menu homebrew : il s'ouvre en **Cartes**, avec ton ordre et tes pages
- [ ] Trois points (en haut Ã  gauche) : tuiles **HomeNX+**, **Style**, **Films** prÃ©sentes
- [ ] Style > Â« Passer Ã  l'apparence Wii U Â» : le .nro se relance en Wii U ; RÃ©glages > HomeNX+ > Â« Passer aux Cartes Â» : retour en Cartes
- [ ] HomeNX+ > Claude > Poser une question : la rÃ©ponse s'affiche dans la fenÃªtre (haut/bas pour dÃ©filer, B pour fermer)
- [ ] + sur un jeu > Fiche HomeNX+ : ajouter une note, la relire ; crÃ©er la checklist avec Claude
- [ ] HomeNX+ > Pages intelligentes : 3 pages apparaissent (ZL / ZR)
- [ ] HomeNX+ > ScÃ¨nes : Â« Batterie Â» puis Â« Aucune Â»
- [ ] Films > Netflix (PC allumÃ©, **pc_agent 0.2.0 lancÃ©**) : Netflix s'ouvre sur le PC puis Moonlight dÃ©marre

## 1 B. NouveautÃ©s 0.9.1
- [ ] Page **MÃ©dias** (ZL / ZR) : cartes Netflix, Disney+, Prime Video, Spotify, Discord, plus Moonlight, Switchfin, StremNX, TsVitch
- [ ] Carte Discord : le Discord de HomeNX s'ouvre
- [ ] + sur un jeu > Fiche HomeNX+ > ScÃ¨ne de ce jeu : Â« Batterie Â», lancer le jeu (frÃ©quences basses), revenir au menu (retour Ã  la normale)
- [ ] Fiche HomeNX+ > Cocher la checklist : cocher 2 objectifs, refermer, rouvrir : ils restent cochÃ©s
- [ ] Claude > Poser une question : le message Â« Claude rÃ©flÃ©chitâ€¦ Â» reste affichÃ© jusqu'Ã  la rÃ©ponse

## 1 C. NouveautÃ©s 0.9.2
- [ ] Sur l'accueil, **Y** : taper Â« zel Â» : les jeux dont le nom contient Â« zel Â» s'affichent ; A amÃ¨ne sur la carte
- [ ] Clavier USB branchÃ© (dock) : taper une lettre ouvre la recherche
- [ ] Clic du stick gauche : le panneau HomeNX+ s'ouvre
- [ ] Page MÃ©dias : SwitchWave, GMCA, NXMilk et NewPipe y sont, et se lancent

## 1 D. NouveautÃ©s 0.9.3
- [ ] Panneau HomeNX+ > Ranger mes jeux avec Claude : une proposition de pages s'affiche ; Â« CrÃ©er ces pages Â» : elles apparaissent (ZL / ZR)
- [ ] Page MÃ©dias > Jeux du PC (PC en veille, Wake-on-LAN actif) : le PC se rÃ©veille, puis Moonlight dÃ©marre
- [ ] Apparence Wii U > RÃ©glages > HomeNX+ > MÃ©dias : Netflix ouvre le site sur le PC puis Moonlight

## 1 E. Nouveautés 0.9.4
- [ ] Premier lancement : l'écran « Premiers pas » s'ouvre (une seule fois)
- [ ] Panneau > Statistiques de jeu : total, top 10, joués récemment
- [ ] Panneau > Claude > Poser une question : cases jeu / notes / capture, puis réponse
- [ ] + sur un jeu > Fiche > Mes captures de ce jeu
- [ ] Panneau > Thème nuit automatique (le soir : verre sombre)
- [ ] Panneau > Mettre à jour HomeNX+ : « à jour (0.9.4) »
- [ ] Page Médias : 16 services (Netflix… Apple Music)

## 1 F. Nouveautés 0.9.5
- [ ] Panneau > Résumé de la semaine (après quelques jours) ; « Envoyer le résumé au PC »
- [ ] Panneau > Rappel de pause : 1 h ; jouer plus d'1 h, revenir au menu : message « pense à faire une pause »
- [ ] Panneau > Sauvegarde auto des saves (PC allumé, pc_agent lancé) : au prochain retour au menu, copie vers le PC
- [ ] Panneau > Claude > Demander une action : « mets la scène Nuit » → Claude propose, « Oui, fais-le » applique

## 1 ter. Onglet Â« HomeNX+ Â» (dans HomeNX+ : RÃ©glages, dernier onglet Ã  droite)
- [ ] L'onglet **HomeNX+** apparaÃ®t aprÃ¨s Â« HomeNX Â»
- [ ] Claude : Â« ClÃ© API : PrÃ©sente Â» une fois la clÃ© mise ; Â« Ouvrir ClaudeNX Â» lance ClaudeNX
- [ ] ScÃ¨nes : choisir **Nuit** (la luminositÃ© baisse), puis **Perf max** (Status Monitor montre CPU 1785 / GPU 921), puis **Aucune** (sys-clk reprend la main)
- [ ] Batterie : le pourcentage affichÃ© correspond Ã  celui de la console
- [ ] Rangement : **Importer** affiche Â« ImportÃ© : 55 jeux, 8 masquÃ©s Â»

## 1 quater. Lien avec le PC (onglet HomeNX+ > PC)
Sur le PC d'abord : `.\pc_agent\lancer_pc_agent.ps1` (accepter Python sur le rÃ©seau **privÃ©** si Windows le demande), puis `.\outils\installer_sur_sd.ps1` avec la carte dans l'Ally.
- [ ] Â« Lien avec le PC : ConnectÃ© (adresse du PC) Â»
- [ ] Â« Envoyer un message au PC Â» : le fichier arrive dans `Documents\Switch_Sauvegardes\boite\de_switch`
- [ ] DÃ©poser un fichier `.json` dans `Documents\Switch_Sauvegardes\boite\vers_switch`, puis Â« Messages du PC Â» > Lire
- [ ] Â« RÃ©veiller le PC Â», PC en veille : il ne se rÃ©veillera sans doute pas en Wi-Fi (le Wake-on-LAN marche surtout en cÃ¢ble Ethernet)

## 1 sexies. Dernier jeu et dossiers intelligents (onglet HomeNX+)
- [ ] Section Â« Dernier jeu : <nom> Â» : le bon jeu s'affiche
- [ ] Â« Mes notes Â» > Modifier : Ã©crire une note, puis la retrouver dans l'overlay en jeu
- [ ] Â« CrÃ©er avec Claude Â» (avec la clÃ© API) : Â« N objectifs enregistrÃ©s Â», puis les cocher dans l'overlay
- [ ] Dossiers intelligents : RÃ©cents, Les plus jouÃ©s (avec les heures) et Jamais lancÃ©s sont cohÃ©rents
- [ ] Â« Traduire ma derniÃ¨re capture Â» (clÃ© API) : la traduction s'affiche et apparaÃ®t dans les notes (overlay)
- [ ] Â« Copier ses saves sur le PC Â» (pc_agent lancÃ©, jeu fermÃ©) : les fichiers arrivent dans `Documents\Switch_Sauvegardes\<date>\saves`
- [ ] ScÃ¨ne **Nuit** : le filtre Fizeau s'allume ; scÃ¨ne **Aucune** : il s'Ã©teint

## 1 quinquies. Overlay HomeNX+ en jeu
- [ ] Dans un jeu : **L + Bas + Stick droit** (Ultrahand), puis l'overlay **HomeNX+** dans la liste
- [ ] Batterie : le pourcentage est correct
- [ ] ScÃ¨nes : choisir **Perf max**, puis vÃ©rifier dans Status Monitor ; revenir Ã  **Aucune**
- [ ] Notes : sans fichier de notes, Â« Aucune note Â» s'affiche (sans plantage)
- [ ] Section PC du menu : **Envoyer mes captures au PC** (pc_agent lancÃ©) â†’ les JPEG arrivent dans `Documents\Switch_Sauvegardes\<date>\captures`

## 1 bis. ClaudeNX (Claude sur la Switch)
Avant : crÃ©er une clÃ© API sur **console.anthropic.com** (facturÃ©e Ã  part de Claude Pro), avec une **limite de dÃ©penses** (par exemple 5 â‚¬ par mois). La mettre seule dans `sd:/config/homenxplus/api_key.txt` (carte dans l'Ally).
- [ ] Lancer **ClaudeNX** depuis le menu homebrew : l'Ã©cran affiche le modÃ¨le (`claude-haiku-5-5`) et la dÃ©pense du mois
- [ ] **A**, Ã©crire Â« Bonjour Â», valider : la rÃ©ponse s'affiche au fur et Ã  mesure, puis le coÃ»t
- [ ] **X** : passer Ã  Opus, poser une question, revenir avec X
- [ ] **B** pendant une rÃ©ponse : elle s'arrÃªte
- [ ] Sans Wi-Fi : un message clair en franÃ§ais s'affiche
- [ ] Les conversations apparaissent dans `sd:/homenxplus/chats/`
- [ ] Prendre une capture dans un jeu, ouvrir ClaudeNX, **ZR** (Â« capture jointe Â»), puis **A** Â« Que dois-je faire ici ? Â» : Claude dÃ©crit la capture

## 2. ChaÃ®ne de test sans retirer la carte
- [ ] Lire l'adresse IP de la Switch : ParamÃ¨tres > Internet > Ã‰tat de la connexion
- [ ] Activer sys-ftpd (Ultrahand > Sysmodules), puis sur le PC : `.\logs.ps1 -Ip <IP>`
- [ ] DÃ©sactiver sys-ftpd Ã  la fin

## 2 bis. Outils Linux (sous Kubuntu)
- [ ] `bash "/media/<utilisateur>/SWITCH SD/switchroot/outils/homenxplus/installer.sh"` : 4 icÃ´nes HomeNX+ apparaissent
- [ ] IcÃ´ne Â« santÃ© de la Switch Â» : batterie, tempÃ©ratures, mÃ©moire et espace SD s'affichent
- [ ] `boite_aux_lettres.sh envoyer "test"` : un fichier apparaÃ®t dans `homenxplus/mailbox/outbox`

## 3. Secours (Ã  connaÃ®tre, Ã  ne tester qu'avant la phase 12)
- [ ] Sous Kubuntu : `bash "/media/<utilisateur>/SWITCH SD/switchroot/outils/homenxplus/secours_menu_home.sh"` puis `â€¦ reactiver`
