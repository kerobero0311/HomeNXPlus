#!/bin/bash
# installer.sh : ajoute au menu de Kubuntu (et au Bureau) les outils HomeNX+ :
#   Sante de la Switch, Boite aux lettres, Secours menu HOME (desactiver / reactiver).
# A lancer une fois :  bash installer.sh
set -u
ICI="$(dirname "$(readlink -f "$0")")"
APPS="$HOME/.local/share/applications"; mkdir -p "$APPS"
BUREAU="$(xdg-user-dir DESKTOP 2>/dev/null || echo "$HOME/Desktop")"

ecrire() {  # nom fichier commande icone
  cat > "$APPS/$2.desktop" <<EOF
[Desktop Entry]
Type=Application
Name=$1
Exec=konsole --hold -e bash -c '$3'
Icon=$4
Terminal=false
Categories=System;
EOF
  chmod +x "$APPS/$2.desktop"
  if [ -d "$BUREAU" ]; then cp "$APPS/$2.desktop" "$BUREAU/"; chmod +x "$BUREAU/$2.desktop"; gio set "$BUREAU/$2.desktop" metadata::trusted true 2>/dev/null || true; fi
}

ecrire "HomeNX+ : sante de la Switch"       hnp-sante     "bash \"$ICI/rapport_sante.sh\""                     utilities-system-monitor
ecrire "HomeNX+ : boite aux lettres"         hnp-boite     "bash \"$ICI/boite_aux_lettres.sh\" lire"            mail-message-new
ecrire "HomeNX+ : SECOURS menu HOME (off)"   hnp-secours   "bash \"$ICI/secours_menu_home.sh\" desactiver"      dialog-warning
ecrire "HomeNX+ : remettre le menu HOME"     hnp-reactiver "bash \"$ICI/secours_menu_home.sh\" reactiver"       view-refresh
update-desktop-database "$APPS" 2>/dev/null || true
echo "OK : 4 icones HomeNX+ ajoutees (menu Demarrer > Systeme, et sur le Bureau)."
