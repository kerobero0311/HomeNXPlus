# Crédits — HomeNX+

HomeNX+ est une version modifiée de **HomeNX**, elle-même tirée de **SwitchU**.
Licence : **GNU GPL v3.0** (voir `LICENSE`), comme les projets d'origine.

| Projet | Auteur | Licence | Ce qui vient de lui |
|---|---|---|---|
| [HomeNX](https://github.com/ad3333m/homenx) (HomeNX, CardsNX, NXToggle, HomeNX Loader) | ad3333m (Adam) et contributeurs | GPL-3.0 | base du code (fork au build 213) |
| [SwitchU](https://github.com/PoloNX/SwitchU) | PoloNX | GPL-3.0 | projet d'origine de HomeNX : daemon qlaunch, menu, nxui |
| [switch-repo](https://github.com/PoloNX/switch-repo) | PoloNX | — | modèle de paquet `switch-pkg` (copie locale modifiée dans `packages/s/switch-pkg`) |
| [SwitchU, fork de tomvita](https://github.com/tomvita/SwitchU) | tomvita | GPL-3.0 | correctifs de stabilité à reprendre en phase 1 |
| [libtesla](https://github.com/WerWolv/libtesla) (via HomeNX) | WerWolv | GPL-2.0 ou ultérieure | cadre de l'overlay HomeNXPlus.ovl |
| [nlohmann/json](https://github.com/nlohmann/json) | Niels Lohmann | MIT | JSON (copie dans lib/nlohmann pour l'overlay) |

Les composants tiers déjà embarqués par HomeNX (SDL2, FreeType, eSpeak NG, Atmosphere-libs, polices, sons…) restent sous leurs licences : voir `THIRD_PARTY_NOTICES.md`.

Inspiration seulement, sans code repris : uLaunch (XorTroll, GPL-2.0), sLaunch (etonedemid), ChatNX (Jonathan8520), Pocket DeepChat (zerob13).
