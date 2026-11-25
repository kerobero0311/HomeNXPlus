# pc_agent : compagnon PC de HomeNX+ (Windows, Python 3, bibliotheque standard uniquement).
#
# Petit serveur HTTP sur le reseau LOCAL seulement, protege par un jeton partage :
#   GET  /ping                     -> {"ok": true, "version": ...}            (sans jeton)
#   GET  /boite                    -> messages PC -> Switch en attente         (jeton)
#   POST /boite   {"texte": "..."} -> message Switch -> PC                     (jeton)
#   PUT  /depot/<categorie>/<nom>  -> enregistre un fichier (saves, captures)  (jeton)
#   GET  /semaine                  -> resume de la semaine (temps de jeu, sante) (jeton)
#   GET  /reveil                   -> adresse MAC du PC pour le Wake-on-LAN    (jeton)
#   POST /ouvrir  {"service": "netflix"} -> ouvre ce site sur le PC (liste fermee) (jeton)
#                                     puis la Switch lance Moonlight pour le regarder
#
# Lancement :  py -I pc_agent\agent.py            (premier lancement : cree la config et le jeton)
# Donnees   :  %USERPROFILE%\Documents\Switch_Sauvegardes\  (depots, boite aux lettres)
# Config    :  %APPDATA%\HomeNXPlus\agent.json  (port, jeton) - le jeton n'est jamais journalise.
import csv
import datetime as dt
import hmac
import io
import ipaddress
import json
import os
import pathlib
import re
import secrets
import subprocess
import sys
import threading
import time
import urllib.parse
import uuid
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

VERSION = "0.2.0"
TAILLE_MAX = 64 * 1024 * 1024          # 64 Mo par fichier depose
CATEGORIES = {"saves", "captures", "notes", "divers"}
# Streaming : la Switch n'a pas d'appli Netflix / Disney+ / Prime Video (et son navigateur n'a pas
# le DRM Widevine) : le PC ouvre le site, la Switch le regarde par Moonlight.
# Liste FERMEE : la Switch ne peut demander que ces adresses, rien d'autre ne s'execute.
SERVICES = {
    "netflix": "https://www.netflix.com/browse",
    "disney": "https://www.disneyplus.com/",
    "prime": "https://www.primevideo.com/",
    "spotify": "https://open.spotify.com/",
}


def ouvrir_sur_pc(url: str) -> None:
    """Ouvre l'adresse dans le navigateur par defaut du PC."""
    if hasattr(os, "startfile"):
        os.startfile(url)  # noqa: S606 (adresse de la liste SERVICES uniquement)
    else:
        import webbrowser
        webbrowser.open(url)


# ------------------------------------------------------------------ configuration
def dossier_config() -> pathlib.Path:
    return pathlib.Path(os.environ.get("APPDATA", pathlib.Path.home())) / "HomeNXPlus"


def dossier_donnees() -> pathlib.Path:
    return pathlib.Path.home() / "Documents" / "Switch_Sauvegardes"


def charger_config(chemin: pathlib.Path | None = None) -> dict:
    chemin = chemin or dossier_config() / "agent.json"
    cfg = {}
    if chemin.exists():
        try:
            cfg = json.loads(chemin.read_text(encoding="utf-8"))
        except (OSError, ValueError):
            cfg = {}
    modifie = False
    if not isinstance(cfg.get("jeton"), str) or len(cfg["jeton"]) < 32:
        cfg["jeton"] = secrets.token_urlsafe(32)
        modifie = True
    if not isinstance(cfg.get("port"), int) or not (1024 <= cfg["port"] <= 65535):
        cfg["port"] = 8642
        modifie = True
    if modifie:
        chemin.parent.mkdir(parents=True, exist_ok=True)
        chemin.write_text(json.dumps(cfg, indent=1), encoding="utf-8")
    return cfg


# ------------------------------------------------------------------ securite
def ip_autorisee(ip: str) -> bool:
    """Reseau local uniquement (prive ou boucle locale)."""
    try:
        a = ipaddress.ip_address(ip)
    except ValueError:
        return False
    return a.is_private or a.is_loopback


def jeton_valide(recu: str | None, attendu: str) -> bool:
    return bool(recu) and hmac.compare_digest(recu.encode(), attendu.encode())


_NOM_SUR = re.compile(r"^[A-Za-z0-9 _\-.()\[\]~]{1,120}$")  # ~ : dossiers aplatis des saves


def chemin_depot(base: pathlib.Path, categorie: str, nom: str, jour: str) -> pathlib.Path | None:
    """Chemin de destination d'un depot, ou None si la demande est suspecte (pas de ../, pas de C:)."""
    if categorie not in CATEGORIES or not _NOM_SUR.match(nom) or nom in {".", ".."} or nom.startswith("."):
        return None
    dossier = (base / jour / categorie).resolve()
    cible = (dossier / nom).resolve()
    if cible.parent != dossier:
        return None
    return cible


# ------------------------------------------------------------------ boite aux lettres
def ajouter_message(dossier: pathlib.Path, de: str, texte: str) -> pathlib.Path:
    dossier.mkdir(parents=True, exist_ok=True)
    # horodatage en nanosecondes : l'ordre alphabetique = l'ordre d'arrivee, meme dans la meme seconde
    nom = dt.datetime.now().strftime("%Y%m%d-%H%M%S") + f"-{time.time_ns():020d}-{de}.json"
    tmp = dossier / (nom + ".tmp")
    tmp.write_text(json.dumps({"de": de, "date": dt.datetime.now().isoformat(timespec="seconds"), "texte": texte},
                              ensure_ascii=False), encoding="utf-8")
    tmp.replace(dossier / nom)
    return dossier / nom


def lire_messages(dossier: pathlib.Path, archiver: pathlib.Path | None = None) -> list[dict]:
    """Messages en attente (les plus anciens d'abord) ; deplaces dans `archiver` une fois lus."""
    out = []
    if not dossier.exists():
        return out
    for f in sorted(dossier.glob("*.json")):
        try:
            out.append(json.loads(f.read_text(encoding="utf-8")))
        except (OSError, ValueError):
            continue
        if archiver:
            archiver.mkdir(parents=True, exist_ok=True)
            f.replace(archiver / f.name)
    return out


# ------------------------------------------------------------------ resume de la semaine
def resume_semaine(sante_csv: str, temps_csv: str, aujourd_hui: dt.date | None = None) -> dict:
    """sante_csv : homenxplus/health/log.csv ; temps_csv : lignes 'date;titre;minutes'."""
    aujourd_hui = aujourd_hui or dt.date.today()
    debut = aujourd_hui - dt.timedelta(days=6)

    def dans_semaine(texte_date: str) -> bool:
        try:
            return debut <= dt.date.fromisoformat(texte_date[:10]) <= aujourd_hui
        except ValueError:
            return False

    jeux: dict[str, int] = {}
    for ligne in csv.reader(io.StringIO(temps_csv), delimiter=";"):
        if len(ligne) >= 3 and dans_semaine(ligne[0]):
            try:
                jeux[ligne[1]] = jeux.get(ligne[1], 0) + int(ligne[2])
            except ValueError:
                pass
    batteries = []
    for ligne in csv.DictReader(io.StringIO(sante_csv), delimiter=";"):
        if dans_semaine(ligne.get("date", "")):
            try:
                batteries.append(int(ligne.get("batterie", "")))
            except ValueError:
                pass
    top = sorted(jeux.items(), key=lambda kv: -kv[1])[:5]
    total = sum(jeux.values())
    return {
        "du": debut.isoformat(), "au": aujourd_hui.isoformat(),
        "minutes_total": total,
        "jeux": [{"titre": t, "minutes": m} for t, m in top],
        "batterie_min": min(batteries) if batteries else None,
        "texte": (f"Cette semaine : {total // 60} h {total % 60:02d} de jeu"
                  + (f", surtout {top[0][0]}" if top else "")
                  + (f". Batterie descendue jusqu'a {min(batteries)} %." if batteries else ".")),
    }


# ------------------------------------------------------------------ reveil (Wake-on-LAN)
def adresse_mac() -> str:
    """Adresse MAC de la carte reseau principale, format AA:BB:CC:DD:EE:FF."""
    try:  # carte qui porte la route par defaut (Windows)
        sortie = subprocess.run(["powershell", "-NoProfile", "-Command",
                                 "(Get-NetAdapter | Where-Object { $_.Status -eq 'Up' -and $_.HardwareInterface } | "
                                 "Sort-Object -Property @{e={$_.Name -notmatch 'Wi-?Fi'}} | Select-Object -First 1).MacAddress"],
                                capture_output=True, text=True, timeout=15).stdout.strip()
        if re.fullmatch(r"([0-9A-Fa-f]{2}[-:]){5}[0-9A-Fa-f]{2}", sortie):
            return sortie.replace("-", ":").upper()
    except (OSError, subprocess.SubprocessError):
        pass
    n = uuid.getnode()
    return ":".join(f"{(n >> s) & 0xFF:02X}" for s in range(40, -1, -8))


# ------------------------------------------------------------------ serveur HTTP
class Agent(BaseHTTPRequestHandler):
    server_version = "HomeNXPlus-pc_agent/" + VERSION
    cfg: dict = {}
    base: pathlib.Path = dossier_donnees()
    ouvrir = staticmethod(ouvrir_sur_pc)  # remplacable dans les tests

    def log_message(self, fmt, *args):  # journal sans jeton ni contenu
        sys.stderr.write(f"{dt.datetime.now():%H:%M:%S} {self.client_address[0]} {self.command} {self.path.split('?')[0]}\n")

    def _json(self, code: int, obj) -> None:
        data = json.dumps(obj, ensure_ascii=False).encode("utf-8")
        self.send_response(code)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def _autorise(self, besoin_jeton: bool = True) -> bool:
        if not ip_autorisee(self.client_address[0]):
            self._json(403, {"erreur": "reseau local uniquement"})
            return False
        if besoin_jeton and not jeton_valide(self.headers.get("X-HNP-Jeton"), self.cfg["jeton"]):
            self._json(401, {"erreur": "jeton invalide"})
            return False
        return True

    def do_GET(self):
        if self.path == "/ping":
            if self._autorise(False):
                self._json(200, {"ok": True, "version": VERSION})
            return
        if not self._autorise():
            return
        if self.path == "/boite":
            msgs = lire_messages(self.base / "boite" / "vers_switch", self.base / "boite" / "archive")
            self._json(200, {"messages": msgs})
        elif self.path == "/semaine":
            sante = self.base / "sante" / "log.csv"
            temps = self.base / "sante" / "temps_de_jeu.csv"
            lire = lambda p: p.read_text(encoding="utf-8") if p.exists() else ""
            self._json(200, resume_semaine(lire(sante), lire(temps)))
        elif self.path == "/reveil":
            self._json(200, {"mac": adresse_mac()})
        else:
            self._json(404, {"erreur": "inconnu"})

    def do_POST(self):
        if not self._autorise():
            return
        if self.path == "/ouvrir":
            self._ouvrir(); return
        if self.path != "/boite":
            self._json(404, {"erreur": "inconnu"}); return
        n = int(self.headers.get("Content-Length", "0") or 0)
        if n <= 0 or n > 64 * 1024:
            self._json(413, {"erreur": "message trop long"}); return
        try:
            texte = json.loads(self.rfile.read(n).decode("utf-8")).get("texte", "")
        except (ValueError, UnicodeDecodeError, AttributeError):
            self._json(400, {"erreur": "JSON invalide"}); return
        if not isinstance(texte, str) or not texte.strip():
            self._json(400, {"erreur": "texte vide"}); return
        f = ajouter_message(self.base / "boite" / "de_switch", "switch", texte[:8000])
        self._json(201, {"ok": True, "fichier": f.name})

    def _ouvrir(self):
        n = int(self.headers.get("Content-Length", "0") or 0)
        if n <= 0 or n > 1024:
            self._json(400, {"erreur": "demande invalide"}); return
        try:
            service = json.loads(self.rfile.read(n).decode("utf-8")).get("service", "")
        except (ValueError, UnicodeDecodeError, AttributeError):
            self._json(400, {"erreur": "JSON invalide"}); return
        url = SERVICES.get(service) if isinstance(service, str) else None
        if not url:
            self._json(400, {"erreur": "service inconnu", "services": sorted(SERVICES)}); return
        try:
            self.ouvrir(url)
        except OSError as e:
            self._json(500, {"erreur": f"ouverture impossible : {e.__class__.__name__}"}); return
        self._json(200, {"ok": True, "service": service})

    def do_PUT(self):
        if not self._autorise():
            return
        m = re.fullmatch(r"/depot/([a-z]+)/([^/]+)", self.path)
        n = int(self.headers.get("Content-Length", "0") or 0)
        nom = urllib.parse.unquote(m.group(2)) if m else ""  # le client encode les espaces (%20)
        cible = chemin_depot(self.base, m.group(1), nom, dt.date.today().isoformat()) if m else None
        if cible is None:
            self._json(400, {"erreur": "chemin refuse"}); return
        if n <= 0 or n > TAILLE_MAX:
            self._json(413, {"erreur": "fichier vide ou trop gros"}); return
        cible.parent.mkdir(parents=True, exist_ok=True)
        tmp = cible.with_name(cible.name + ".tmp")
        restant = n
        with open(tmp, "wb") as out:
            while restant:
                bloc = self.rfile.read(min(restant, 1 << 20))
                if not bloc:
                    break
                out.write(bloc); restant -= len(bloc)
        if restant:
            tmp.unlink(missing_ok=True)
            self._json(400, {"erreur": "transfert incomplet"}); return
        tmp.replace(cible)
        self._json(201, {"ok": True, "chemin": str(cible.relative_to(self.base))})


def creer_serveur(cfg: dict, base: pathlib.Path, hote: str = "0.0.0.0") -> ThreadingHTTPServer:
    classe = type("AgentConfigure", (Agent,), {"cfg": cfg, "base": base})
    return ThreadingHTTPServer((hote, cfg["port"]), classe)


def main() -> None:
    cfg = charger_config()
    base = dossier_donnees()
    (base / "boite" / "vers_switch").mkdir(parents=True, exist_ok=True)
    srv = creer_serveur(cfg, base)
    print(f"pc_agent HomeNX+ {VERSION} : port {cfg['port']}, donnees dans {base}")
    print(f"Jeton : voir {dossier_config() / 'agent.json'} (a copier dans config/homenxplus/pc_agent.ini de la carte)")
    print("Ctrl+C pour arreter.")
    try:
        srv.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        srv.server_close()


if __name__ == "__main__":
    main()
