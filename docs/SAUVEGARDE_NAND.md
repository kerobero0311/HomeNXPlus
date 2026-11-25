# Sauvegarde de la mémoire interne (NAND sysMMC) — guide pas à pas

**Pourquoi** : c'est la seule copie de secours de la console elle-même. Si la mémoire interne s'abîme,
on peut la restaurer avec cette sauvegarde. Elle n'a jamais été faite sur ta Switch.

**Durée** : 30 à 60 minutes. **Place** : environ 30 Go au total. Ta carte n'a que 13 Go libres :
Hekate fera la sauvegarde **en plusieurs fois**, et il faudra vider la carte sur le PC entre chaque partie.

> Ce qui peut mal tourner : presque rien, Hekate **lit** la mémoire sans rien y écrire.
> Le seul risque est de manquer de batterie pendant la copie : **branche le chargeur officiel**.

## 1. Sur la Switch
1. Branche le chargeur officiel.
2. Démarre sur le menu **Hekate** (injection si la console était éteinte).
3. Touche **Tools**, puis **Backup eMMC**.
4. Touche **eMMC BOOT0 & BOOT1** et attends « Finished ». Ces fichiers sont petits (8 Mo).
5. Touche **eMMC RAW GPP** : c'est la grosse sauvegarde (environ 29 Go).
   - Quand la carte est pleine, Hekate affiche un message du genre « Partial backup… continue »
     et te demande de **vider la carte**.
   - Éteins avec **Power Off** dans Hekate, ou laisse la console sur ce message.

## 2. Sur le PC (à chaque fois que Hekate demande de vider la carte)
1. Mets la carte dans l'Ally.
2. Lance, depuis le dossier HomeNXPlus :
   ```powershell
   .\outils\vider_sauvegarde_nand.ps1
   ```
   Le script **déplace** les morceaux `rawnand.bin.XX` vers `Documents\Sauvegarde_NAND_Switch\<date>\`
   en vérifiant chaque copie (SHA-256) avant de libérer la carte. Il ne touche à rien d'autre.
3. Remets la carte dans la Switch, retourne dans **Tools > Backup eMMC > eMMC RAW GPP** :
   Hekate **reprend où il s'était arrêté**.
4. Recommence jusqu'à « Finished ».

## 3. À la fin
- Garde le dossier `Documents\Sauvegarde_NAND_Switch\<date>\` en lieu sûr, idéalement aussi sur D:.
- Ces fichiers sont **personnels** (ils contiennent les clés de ta console) : ne les partage jamais.
