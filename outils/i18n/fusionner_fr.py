# fusionner_fr.py : ajoute les traductions de fr_ajouts.json (cles a plat "a.b.c") dans
# romfs/i18n/fr-FR.json (cles imbriquees), SANS ecraser une traduction existante.
# Usage : py -I outils/i18n/fusionner_fr.py <racine du projet>
import json, sys, pathlib

root = pathlib.Path(sys.argv[1])
cible = root / "romfs/i18n/fr-FR.json"
fr = json.load(open(cible, encoding="utf-8"))
ajouts = json.load(open(root / "outils/i18n/fr_ajouts.json", encoding="utf-8"))

ajoutes, deja, conflits = 0, 0, []
for cle, val in ajouts.items():
    parts = cle.split(".")
    d = fr
    ok = True
    for p in parts[:-1]:
        if p not in d:
            d[p] = {}
        if not isinstance(d[p], dict):
            ok = False
            break
        d = d[p]
    if not ok or isinstance(d.get(parts[-1]), dict):
        conflits.append(cle)
        continue
    if parts[-1] in d:
        deja += 1
        continue
    d[parts[-1]] = val
    ajoutes += 1

tmp = cible.with_suffix(".json.tmp")
with open(tmp, "w", encoding="utf-8", newline="\n") as f:
    json.dump(fr, f, ensure_ascii=False, indent=2)
    f.write("\n")
tmp.replace(cible)
print(f"ajoutees: {ajoutes}  deja presentes: {deja}  conflits: {len(conflits)} {conflits[:10]}")
