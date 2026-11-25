# HomeNX+

Menu HOME personnel pour Nintendo Switch moddÃ©e (AtmosphÃ¨re), en franÃ§ais, avec Claude intÃ©grÃ©.
Version modifiÃ©e de [HomeNX](https://github.com/ad3333m/homenx) (ad3333m), elle-mÃªme tirÃ©e de
[SwitchU](https://github.com/PoloNX/SwitchU) (PoloNX). Licence **GPL-3.0**. CrÃ©dits : `CREDITS.md`.
Le README d'origine est conservÃ© dans `README_HomeNX.md`.

> Ã‰tat : **en dÃ©veloppement** (version 0.9.3). HomeNX+ se teste en **application** (`.nro`) ;
> l'installation comme vrai menu HOME remplace HomeNX (sauvegarde et plan de secours d'abord).

**In English (short):** HomeNX+ is a French fork of HomeNX for modded Switch consoles (AtmosphÃ¨re).
It adds a HomeNX+ panel to the CardsNX look (and a Style button to switch between the CardsNX and
Wii U looks), per-game notes and completion checklists, performance "scenes" (sys-clk, Fizeau,
brightness), a safe mode, a Claude client (`ClaudeNX.nro`, Anthropic API, your own key), a Tesla
overlay, a small PC companion (`pc_agent`, LAN only: Wake-on-LAN, mailbox, save/screenshot backup,
opening Netflix / Disney+ / Prime Video / Spotify on the PC to watch them through Moonlight), and Linux
(L4T) helper scripts. Builds natively on Windows (devkitPro + xmake). GPL-3.0, see `CREDITS.md`.
No keys, firmware, game files or signature patches are included or downloaded.

## Ce qu'il y a sur la carte SD

| Chemin | RÃ´le |
|---|---|
| `switch/HomeNXPlus/HomeNXPlus.nro` | HomeNX+ en application de test (Ã  ouvrir depuis le menu homebrew) |
| `switch/ClaudeNX/ClaudeNX.nro` | Claude en application autonome |
| `config/homenxplus/` | rÃ©glages : `config.ini`, `api_key.txt` (ta clÃ©, jamais affichÃ©e), `cacert.pem` ; `menu/` = rÃ©glages du menu HomeNX+ |
| `homenxplus/` | donnÃ©es : notes, checklists, journauxâ€¦ |
| `switchroot/outils/homenxplus/` | scripts Linux (secoursâ€¦) |

## Utiliser la version de test
1. Ouvre le menu homebrew (album, ou bouton Homebrew de HomeNX).
2. Lance **HomeNX+**. Pour en sortir : bouton HOME.

## ClaudeNX (Claude sur la Switch)
1. CrÃ©e une clÃ© API sur console.anthropic.com, avec une limite de dÃ©penses (facturÃ©e Ã  part de Claude Pro).
2. Mets-la, seule, dans `config/homenxplus/api_key.txt` sur la carte.
3. Lance **ClaudeNX** : **A** pour poser une question, **X** pour changer de modÃ¨le (Haiku â†” Opus), **Y** pour une nouvelle conversation, **B** pour arrÃªter une rÃ©ponse, **+** pour quitter.
4. Les rÃ©glages (modÃ¨le, plafond mensuel en â‚¬â€¦) sont dans `config/homenxplus/config.ini`.

## Installer depuis le PC (carte dans le lecteur)
```powershell
.\outils\installer_sur_sd.ps1
```

## Mettre Ã  jour (depuis le PC)
```powershell
.\build.ps1 -Variant homebrew              # compile
.\test.ps1                                 # tests automatiques
.\deploy.ps1 -Ip <IP de la Switch>         # envoi FTP (activer sys-ftpd avant, le couper aprÃ¨s)
```
Sans FTP : carte dans le lecteur de l'Ally, ou mode UMS (Ultrahand > Changer de systÃ¨me > Carte SD sur le PC).

## Secours (si un menu HOME perso empÃªche la console de dÃ©marrer)
- **Depuis le PC** (carte dans le lecteur ou en UMS) : `.\secours_menu_home.ps1`
- **Depuis Kubuntu** : `bash /media/$USER/SWITCH\ SD/switchroot/outils/homenxplus/secours_menu_home.sh`

Les deux renomment `atmosphere/contents/0100000000001000/exefs.nsp` en `exefs.nsp.disabled` : la console
redÃ©marre avec le menu Nintendo. Ajouter `-Action reactiver` (PC) ou `reactiver` (Linux) pour revenir.

## DÃ©sinstaller complÃ¨tement
Supprimer `switch/HomeNXPlus/`, `switch/.overlays/HomeNXPlus.ovl`, `homenxplus/` et `config/homenxplus/`.
Le HomeNX installÃ© n'est pas concernÃ©.

## Compiler (Windows)
PrÃ©requis : devkitPro dans MSYS2 (`C:\devkitPro\msys64\opt\devkitpro`) avec devkitA64, libnx et les portlibs,
xmake (portable, `%USERPROFILE%\xmake`), MinGW de MSYS2 (`ucrt64`), Python 3. `outils/env.ps1` rÃ¨gle tout
pour la session, et `build.ps1` le charge automatiquement.
```powershell
.\build.ps1 -Variant homebrew      # .nro de test   -> build\homebrew\cross\aarch64\release\
.\build.ps1 -NoInstall             # menu HOME + daemon, sans installation
```
Suivi du projet : `PLAN.md`, `DECISIONS.md`, `TESTS_CONSOLE.md`, `VEILLE.md`.
