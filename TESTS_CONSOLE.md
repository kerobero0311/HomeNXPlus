# TESTS CONSOLE — HomeNX+

À faire sur la Switch. Coche quand c'est validé et note ce qui s'est passé.

## 1. HomeNX+ en application de test (déjà sur la carte depuis le 24/11)
Le vrai menu HOME n'est pas modifié : rien ne peut casser le démarrage.
- [ ] Ouvrir le menu homebrew (album, ou carte Homebrew de HomeNX), lancer **HomeNX+**
- [ ] L'interface s'affiche (apparence HomeNX / Wii U) et tes jeux apparaissent
- [ ] Lancer un jeu depuis HomeNX+, puis revenir avec HOME
- [ ] Changer un réglage dans HomeNX+, quitter, et vérifier que **ton HomeNX normal n'a pas changé**
- [ ] Si ça plante : noter l'écran, puis carte dans l'Ally (je lis `homenxplus/logs/` et `atmosphere/crash_reports/`)

## 1 A. Nouveautés 0.9 : apparence Cartes, Style, panneau HomeNX+ (dans HomeNX+.nro)
- [ ] Ouvrir HomeNX+ depuis le menu homebrew : il s'ouvre en **Cartes**, avec ton ordre et tes pages
- [ ] Trois points (en haut à gauche) : tuiles **HomeNX+**, **Style**, **Films** présentes
- [ ] Style > « Passer à l'apparence Wii U » : le .nro se relance en Wii U ; Réglages > HomeNX+ > « Passer aux Cartes » : retour en Cartes
- [ ] HomeNX+ > Claude > Poser une question : la réponse s'affiche dans la fenêtre (haut/bas pour défiler, B pour fermer)
- [ ] + sur un jeu > Fiche HomeNX+ : ajouter une note, la relire ; créer la checklist avec Claude
- [ ] HomeNX+ > Pages intelligentes : 3 pages apparaissent (ZL / ZR)
- [ ] HomeNX+ > Scènes : « Batterie » puis « Aucune »
- [ ] Films > Netflix (PC allumé, **pc_agent 0.2.0 lancé**) : Netflix s'ouvre sur le PC puis Moonlight démarre

## 1 ter. Onglet « HomeNX+ » (dans HomeNX+ : Réglages, dernier onglet à droite)
- [ ] L'onglet **HomeNX+** apparaît après « HomeNX »
- [ ] Claude : « Clé API : Présente » une fois la clé mise ; « Ouvrir ClaudeNX » lance ClaudeNX
- [ ] Scènes : choisir **Nuit** (la luminosité baisse), puis **Perf max** (Status Monitor montre CPU 1785 / GPU 921), puis **Aucune** (sys-clk reprend la main)
- [ ] Batterie : le pourcentage affiché correspond à celui de la console
- [ ] Rangement : **Importer** affiche « Importé : 55 jeux, 8 masqués »

## 1 quater. Lien avec le PC (onglet HomeNX+ > PC)
Sur le PC d'abord : `.\pc_agent\lancer_pc_agent.ps1` (accepter Python sur le réseau **privé** si Windows le demande), puis `.\outils\installer_sur_sd.ps1` avec la carte dans l'Ally.
- [ ] « Lien avec le PC : Connecté (192.168.1.50) »
- [ ] « Envoyer un message au PC » : le fichier arrive dans `Documents\Switch_Sauvegardes\boite\de_switch`
- [ ] Déposer un fichier `.json` dans `Documents\Switch_Sauvegardes\boite\vers_switch`, puis « Messages du PC » > Lire
- [ ] « Réveiller le PC », PC en veille : il ne se réveillera sans doute pas en Wi-Fi (le Wake-on-LAN marche surtout en câble Ethernet)

## 1 sexies. Dernier jeu et dossiers intelligents (onglet HomeNX+)
- [ ] Section « Dernier jeu : <nom> » : le bon jeu s'affiche
- [ ] « Mes notes » > Modifier : écrire une note, puis la retrouver dans l'overlay en jeu
- [ ] « Créer avec Claude » (avec la clé API) : « N objectifs enregistrés », puis les cocher dans l'overlay
- [ ] Dossiers intelligents : Récents, Les plus joués (avec les heures) et Jamais lancés sont cohérents
- [ ] « Traduire ma dernière capture » (clé API) : la traduction s'affiche et apparaît dans les notes (overlay)
- [ ] « Copier ses saves sur le PC » (pc_agent lancé, jeu fermé) : les fichiers arrivent dans `Documents\Switch_Sauvegardes\<date>\saves`
- [ ] Scène **Nuit** : le filtre Fizeau s'allume ; scène **Aucune** : il s'éteint

## 1 quinquies. Overlay HomeNX+ en jeu
- [ ] Dans un jeu : **L + Bas + Stick droit** (Ultrahand), puis l'overlay **HomeNX+** dans la liste
- [ ] Batterie : le pourcentage est correct
- [ ] Scènes : choisir **Perf max**, puis vérifier dans Status Monitor ; revenir à **Aucune**
- [ ] Notes : sans fichier de notes, « Aucune note » s'affiche (sans plantage)
- [ ] Section PC du menu : **Envoyer mes captures au PC** (pc_agent lancé) → les JPEG arrivent dans `Documents\Switch_Sauvegardes\<date>\captures`

## 1 bis. ClaudeNX (Claude sur la Switch)
Avant : créer une clé API sur **console.anthropic.com** (facturée à part de Claude Pro), avec une **limite de dépenses** (par exemple 5 € par mois). La mettre seule dans `sd:/config/homenxplus/api_key.txt` (carte dans l'Ally).
- [ ] Lancer **ClaudeNX** depuis le menu homebrew : l'écran affiche le modèle (`claude-haiku-5-5`) et la dépense du mois
- [ ] **A**, écrire « Bonjour », valider : la réponse s'affiche au fur et à mesure, puis le coût
- [ ] **X** : passer à Opus, poser une question, revenir avec X
- [ ] **B** pendant une réponse : elle s'arrête
- [ ] Sans Wi-Fi : un message clair en français s'affiche
- [ ] Les conversations apparaissent dans `sd:/homenxplus/chats/`
- [ ] Prendre une capture dans un jeu, ouvrir ClaudeNX, **ZR** (« capture jointe »), puis **A** « Que dois-je faire ici ? » : Claude décrit la capture

## 2. Chaîne de test sans retirer la carte
- [ ] Lire l'adresse IP de la Switch : Paramètres > Internet > État de la connexion
- [ ] Activer sys-ftpd (Ultrahand > Sysmodules), puis sur le PC : `.\logs.ps1 -Ip <IP>`
- [ ] Désactiver sys-ftpd à la fin

## 2 bis. Outils Linux (sous Kubuntu)
- [ ] `bash "/media/<utilisateur>/SWITCH SD/switchroot/outils/homenxplus/installer.sh"` : 4 icônes HomeNX+ apparaissent
- [ ] Icône « santé de la Switch » : batterie, températures, mémoire et espace SD s'affichent
- [ ] `boite_aux_lettres.sh envoyer "test"` : un fichier apparaît dans `homenxplus/mailbox/outbox`

## 3. Secours (à connaître, à ne tester qu'avant la phase 12)
- [ ] Sous Kubuntu : `bash "/media/<utilisateur>/SWITCH SD/switchroot/outils/homenxplus/secours_menu_home.sh"` puis `… reactiver`
