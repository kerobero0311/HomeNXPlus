# config_switch.py : ecrit le contenu de config/homenxplus/pc_agent.ini (IP, port, jeton, MAC du PC)
# sur la sortie standard. Utilise par outils/installer_sur_sd.ps1. Ne journalise pas le jeton.
import pathlib
import socket
import sys

sys.path.insert(0, str(pathlib.Path(__file__).parent))
import agent  # noqa: E402


def ip_locale() -> str:
    """IP du PC sur le reseau local (celle qu'utilise la route vers Internet), sans rien envoyer."""
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        s.connect(("192.0.2.1", 9))  # adresse de documentation : aucun paquet n'est envoye pour un connect UDP
        return s.getsockname()[0]
    except OSError:
        return "127.0.0.1"
    finally:
        s.close()


cfg = agent.charger_config()
sys.stdout.write(
    "; Lien HomeNX+ <-> PC (pc_agent). Ecrit par outils/installer_sur_sd.ps1.\n"
    f"ip = {ip_locale()}\nport = {cfg['port']}\njeton = {cfg['jeton']}\nmac = {agent.adresse_mac()}\n")
