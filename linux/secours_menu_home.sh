#!/bin/bash
# secours_menu_home.sh : remet le menu HOME Nintendo d'origine si le menu perso (HomeNX / HomeNX+) bloque.
# A lancer sous Kubuntu (Switch) :  bash secours_menu_home.sh            -> desactive le menu perso
#                                  bash secours_menu_home.sh reactiver  -> le remet
# Methode : renommer exefs.nsp en exefs.nsp.disabled (comme NXToggle). Rien n'est supprime.
set -u
SD="${SD:-/media/$USER/SWITCH SD}"
DIR="$SD/atmosphere/contents/0100000000001000"
LOG="$SD/VERSIONS.txt"

if [ ! -d "$SD/atmosphere" ]; then
  echo "Carte SD introuvable dans $SD. Ouvre-la dans Dolphin (gestionnaire de fichiers) puis relance."
  echo "Ou precise le chemin : SD=/chemin/vers/la/carte bash $0"
  exit 1
fi

note() { printf '\r\n%s - %s' "$(date '+%d/%m/%Y %H:%M')" "$1" >> "$LOG"; }

case "${1:-desactiver}" in
  desactiver)
    if [ -f "$DIR/exefs.nsp" ]; then
      mv "$DIR/exefs.nsp" "$DIR/exefs.nsp.disabled" && sync
      note "Secours (Linux) : menu HOME perso desactive (exefs.nsp -> exefs.nsp.disabled)."
      echo "OK : au prochain demarrage, la console utilisera le menu HOME Nintendo."
    elif [ -f "$DIR/exefs.nsp.disabled" ]; then
      echo "Deja desactive : rien a faire."
    else
      echo "Aucun menu HOME perso trouve dans $DIR : rien a faire."
    fi ;;
  reactiver)
    if [ -f "$DIR/exefs.nsp.disabled" ] && [ ! -f "$DIR/exefs.nsp" ]; then
      mv "$DIR/exefs.nsp.disabled" "$DIR/exefs.nsp" && sync
      note "Secours (Linux) : menu HOME perso reactive (exefs.nsp.disabled -> exefs.nsp)."
      echo "OK : le menu HOME perso reviendra au prochain demarrage."
    else
      echo "Rien a reactiver."
    fi ;;
  *) echo "Usage : bash $0 [desactiver|reactiver]"; exit 1 ;;
esac
