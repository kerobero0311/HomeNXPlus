#!/usr/bin/env bash
# HomeNX+ : filme l'écran de Kubuntu (pour le trailer). Usage : bash filmer_ecran.sh [secondes]
# La vidéo va dans ~/Vidéos, puis une copie part sur la carte SD (homenxplus/tournage/) pour le PC.
set -u
DUREE="${1:-30}"
case "$DUREE" in ''|*[!0-9]*) echo "Durée en secondes, par exemple : bash filmer_ecran.sh 20"; exit 1 ;; esac

if ! command -v ffmpeg >/dev/null 2>&1; then
  echo "ffmpeg n'est pas installé. Installe-le une fois avec :"
  echo "  sudo apt install ffmpeg"
  exit 1
fi
if [ -z "${DISPLAY:-}" ]; then
  echo "Lance ce script depuis le bureau (terminal Konsole), pas en SSH."
  exit 1
fi

TAILLE="$(xdpyinfo 2>/dev/null | awk '/dimensions:/{print $2; exit}')"
[ -z "$TAILLE" ] && TAILLE="1280x720"
DOSSIER="$HOME/Vidéos"
mkdir -p "$DOSSIER"
FICHIER="$DOSSIER/homenxplus_linux_$(date +%Y%m%d_%H%M%S).mp4"

echo "Enregistrement de $DUREE s dans 3 secondes ($TAILLE)..."
sleep 3
ffmpeg -loglevel error -y -f x11grab -framerate 30 -video_size "$TAILLE" -i "$DISPLAY" -t "$DUREE" \
       -c:v libx264 -preset ultrafast -crf 23 -pix_fmt yuv420p "$FICHIER" || { echo "Échec de l'enregistrement."; exit 1; }
echo "Vidéo : $FICHIER"

# Copie sur la carte SD (partition FAT32 « SWITCH SD »), si elle est montée
for SD in "/media/$USER/SWITCH SD" /media/"$USER"/*; do
  if [ -d "$SD/switch" ] && [ -d "$SD/atmosphere" ]; then
    mkdir -p "$SD/homenxplus/tournage" && cp "$FICHIER" "$SD/homenxplus/tournage/" && sync \
      && echo "Copie sur la carte : homenxplus/tournage/$(basename "$FICHIER")"
    break
  fi
done
