#!/bin/bash
# rapport_sante.sh : etat de la Switch sous Kubuntu (batterie, temperatures, carte SD, memoire).
# Lecture seule. Ajoute une ligne a homenxplus/health/log.csv (meme fichier que HomeNX+).
# Usage : bash rapport_sante.sh
set -u
SD="${SD:-/media/$USER/SWITCH SD}"

bat="?"; etat="?"
for b in /sys/class/power_supply/*; do
  [ -f "$b/capacity" ] || continue
  bat="$(cat "$b/capacity" 2>/dev/null)"; etat="$(cat "$b/status" 2>/dev/null)"; break
done

temps=""
for z in /sys/class/thermal/thermal_zone*; do
  [ -f "$z/temp" ] || continue
  t="$(cat "$z/temp" 2>/dev/null)"; n="$(cat "$z/type" 2>/dev/null)"
  [ -n "$t" ] && temps="$temps $n=$((t / 1000))C"
done

mem="?"; command -v free >/dev/null && mem="$(free -m | awk '/^Mem:/ {print $3 "/" $2 " Mo"}')"
sdlibre="?"
[ -d "$SD" ] && sdlibre="$(df -h "$SD" | awk 'NR==2 {print $4}')"

echo "=== Sante de la Switch ($(date '+%d/%m/%Y %H:%M')) ==="
echo "Batterie    : ${bat} % (${etat})"
echo "Temperatures:${temps:- inconnues}"
echo "Memoire     : ${mem}"
echo "Carte SD    : ${sdlibre} libres"
if [ "$bat" != "?" ] && [ "$bat" -le 15 ] && [ "$etat" != "Charging" ]; then
  echo "ATTENTION : batterie faible. Avec l'AutoRCM, une batterie vide oblige a reinjecter Hekate. Branche le chargeur."
fi

if [ -d "$SD" ]; then
  mkdir -p "$SD/homenxplus/health"
  f="$SD/homenxplus/health/log.csv"
  [ -f "$f" ] || echo "date;source;batterie;etat;temperatures;memoire;sd_libre" > "$f"
  echo "$(date '+%Y-%m-%dT%H:%M:%S');linux;$bat;$etat;${temps# };$mem;$sdlibre" >> "$f"
  echo "(ligne ajoutee a homenxplus/health/log.csv)"
fi
