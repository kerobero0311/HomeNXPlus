# HomeNX+

Menu HOME personnel pour Nintendo Switch moddée (Atmosphère), en français, avec Claude intégré.
Version modifiée de [HomeNX](https://github.com/ad3333m/homenx) (ad3333m), elle-même tirée de
[SwitchU](https://github.com/PoloNX/SwitchU) (PoloNX). Licence **GPL-3.0**. Crédits : `CREDITS.md`.
Le README d'origine est conservé dans `README_HomeNX.md`.

> État : **en développement** (version 0.9.3). HomeNX+ se teste en **application** (`.nro`) ;
> l'installation comme vrai menu HOME remplace HomeNX (sauvegarde et plan de secours d'abord).

**In English (short):** HomeNX+ is a French fork of HomeNX for modded Switch consoles (Atmosphère).
It adds a HomeNX+ panel to the CardsNX look (and a Style button to switch between the CardsNX and
Wii U looks), per-game notes and completion checklists, performance "scenes" (sys-clk, Fizeau,
brightness), a safe mode, a Claude client (`ClaudeNX.nro`, Anthropic API, your own key), a Tesla
overlay, a small PC companion (`pc_agent`, LAN only: Wake-on-LAN, mailbox, save/screenshot backup,
opening Netflix / Disney+ / Prime Video / Spotify on the PC to watch them through Moonlight), and Linux
(L4T) helper scripts. Builds natively on Windows (devkitPro + xmake). GPL-3.0, see `CREDITS.md`.
No keys, firmware, game files or signature patches are included or downloaded.

## Ce qu'il y a sur la carte SD

| Chemin | Rôle |
|---|---|
| `switch/HomeNXPlus/HomeNXPlus.nro` | HomeNX+ en application de test (à ouvrir depuis le menu homebrew) |
| `switch/ClaudeNX/ClaudeNX.nro` | Claude en application autonome |
| `config/homenxplus/` | réglages : `config.ini`, `api_key.txt` (ta clé, jamais affichée), `cacert.pem` ; `menu/` = réglages du menu HomeNX+ |
| `homenxplus/` | données : notes, checklists, journaux… |
| `switchroot/outils/homenxplus/` | scripts Linux (secours…) |

## Utiliser la version de test
1. Ouvre le menu homebrew (album, ou bouton Homebrew de HomeNX).
2. Lance **HomeNX+**. Pour en sortir : bouton HOME.

## ClaudeNX (Claude sur la Switch)
1. Crée une clé API sur console.anthropic.com, avec une limite de dépenses (facturée à part de Claude Pro).
2. Mets-la, seule, dans `config/homenxplus/api_key.txt` sur la carte.
3. Lance **ClaudeNX** : **A** pour poser une question, **X** pour changer de modèle (Haiku ↔ Opus), **Y** pour une nouvelle conversation, **B** pour arrêter une réponse, **+** pour quitter.
4. Les réglages (modèle, plafond mensuel en €…) sont dans `config/homenxplus/config.ini`.

## Installer depuis le PC (carte dans le lecteur)
```powershell
.\outils\installer_sur_sd.ps1
```

## Mettre à jour (depuis le PC)
```powershell
.\build.ps1 -Variant homebrew              # compile
.\test.ps1                                 # tests automatiques
.\deploy.ps1 -Ip <IP de la Switch>         # envoi FTP (activer sys-ftpd avant, le couper après)
```
Sans FTP : carte dans le lecteur de l'Ally, ou mode UMS (Ultrahand > Changer de système > Carte SD sur le PC).

## Secours (si un menu HOME perso empêche la console de démarrer)
- **Depuis le PC** (carte dans le lecteur ou en UMS) : `.\secours_menu_home.ps1`
- **Depuis Kubuntu** : `bash /media/$USER/SWITCH\ SD/switchroot/outils/homenxplus/secours_menu_home.sh`

Les deux renomment `atmosphere/contents/0100000000001000/exefs.nsp` en `exefs.nsp.disabled` : la console
redémarre avec le menu Nintendo. Ajouter `-Action reactiver` (PC) ou `reactiver` (Linux) pour revenir.

## Désinstaller complètement
Supprimer `switch/HomeNXPlus/`, `switch/.overlays/HomeNXPlus.ovl`, `homenxplus/` et `config/homenxplus/`.
Le HomeNX installé n'est pas concerné.

## Compiler (Windows)
Prérequis : devkitPro dans MSYS2 (`C:\devkitPro\msys64\opt\devkitpro`) avec devkitA64, libnx et les portlibs,
xmake (portable, `%USERPROFILE%\xmake`), MinGW de MSYS2 (`ucrt64`), Python 3. `outils/env.ps1` règle tout
pour la session, et `build.ps1` le charge automatiquement.
```powershell
.\build.ps1 -Variant homebrew      # .nro de test   -> build\homebrew\cross\aarch64\release\
.\build.ps1 -NoInstall             # menu HOME + daemon, sans installation
```
Suivi du projet : `PLAN.md`, `DECISIONS.md`, `TESTS_CONSOLE.md`, `VEILLE.md`.
