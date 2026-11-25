#!/bin/bash
# boite_aux_lettres.sh : messages entre Linux (Kubuntu) et HomeNX+ (Horizon), via la carte SD.
#   bash boite_aux_lettres.sh lire              -> affiche les messages recus (homenxplus/mailbox/inbox)
#   bash boite_aux_lettres.sh envoyer "texte"   -> depose un message pour HomeNX+ (homenxplus/mailbox/outbox)
#   bash boite_aux_lettres.sh vider             -> archive les messages lus
# Format : un fichier JSON par message {"de":..., "date":..., "texte":...}
set -u
SD="${SD:-/media/$USER/SWITCH SD}"
BOITE="$SD/homenxplus/mailbox"
[ -d "$SD/atmosphere" ] || { echo "Carte SD introuvable ($SD). Ouvre-la dans Dolphin puis relance."; exit 1; }
mkdir -p "$BOITE/inbox" "$BOITE/outbox" "$BOITE/archive"

json_echappe() { python3 -c 'import json,sys; print(json.dumps(sys.argv[1]))' "$1"; }

case "${1:-lire}" in
  lire)
    n=0
    for f in "$BOITE"/inbox/*.json; do
      [ -f "$f" ] || continue; n=$((n + 1))
      python3 - "$f" <<'PY'
import json, sys
m = json.load(open(sys.argv[1], encoding="utf-8"))
print(f"--- de {m.get('de','?')} le {m.get('date','?')}\n{m.get('texte','')}\n")
PY
    done
    [ "$n" -eq 0 ] && echo "Aucun message." ;;
  envoyer)
    texte="${2:-}"; [ -n "$texte" ] || { echo "Usage : bash $0 envoyer \"ton message\""; exit 1; }
    f="$BOITE/outbox/$(date '+%Y%m%d-%H%M%S')-linux.json"
    printf '{"de":"linux","date":"%s","texte":%s}\n' "$(date '+%Y-%m-%dT%H:%M:%S')" "$(json_echappe "$texte")" > "$f.tmp" && mv "$f.tmp" "$f"
    sync; echo "Message depose pour HomeNX+ : $(basename "$f")" ;;
  vider)
    mv "$BOITE"/inbox/*.json "$BOITE/archive/" 2>/dev/null; echo "Messages archives." ;;
  *) echo "Usage : bash $0 [lire|envoyer \"texte\"|vider]"; exit 1 ;;
esac
