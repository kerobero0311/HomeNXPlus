# Tests du pc_agent (unittest, bibliotheque standard). Lances par test.ps1 :  py -I -m unittest
import datetime as dt
import http.client
import json
import pathlib
import sys
import tempfile
import threading
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).parent))
import agent  # noqa: E402


class Logique(unittest.TestCase):
    def test_ip(self):
        for ip in ("192.168.1.20", "192.168.1.50", "127.0.0.1", "172.16.0.5"):
            self.assertTrue(agent.ip_autorisee(ip), ip)
        for ip in ("8.8.8.8", "1.1.1.1", "pas une ip"):
            self.assertFalse(agent.ip_autorisee(ip), ip)

    def test_jeton(self):
        self.assertTrue(agent.jeton_valide("abc", "abc"))
        self.assertFalse(agent.jeton_valide("abd", "abc"))
        self.assertFalse(agent.jeton_valide(None, "abc"))
        self.assertFalse(agent.jeton_valide("", "abc"))

    def test_chemins_depot(self):
        base = pathlib.Path(tempfile.mkdtemp())
        ok = agent.chemin_depot(base, "saves", "Zelda (save).zip", "2026-11-25")
        self.assertEqual(ok, (base / "2026-11-25" / "saves" / "Zelda (save).zip").resolve())
        self.assertIsNotNone(agent.chemin_depot(base, "saves", "Mario Kart__save~slot1.dat", "2026-11-25"))
        for cat, nom in [("saves", "../x"), ("saves", "..\\x"), ("saves", "C:x"), ("saves", ".cache"),
                         ("autre", "a.zip"), ("saves", "a/b"), ("saves", "x" * 200)]:
            self.assertIsNone(agent.chemin_depot(base, cat, nom, "2026-11-25"), (cat, nom))

    def test_config(self):
        d = pathlib.Path(tempfile.mkdtemp()) / "agent.json"
        c1 = agent.charger_config(d)
        self.assertGreaterEqual(len(c1["jeton"]), 32)
        self.assertEqual(c1["port"], 8642)
        self.assertEqual(agent.charger_config(d)["jeton"], c1["jeton"])  # stable
        d.write_text("{abime", encoding="utf-8")
        self.assertGreaterEqual(len(agent.charger_config(d)["jeton"]), 32)

    def test_boite(self):
        d = pathlib.Path(tempfile.mkdtemp())
        agent.ajouter_message(d / "in", "pc", "Bonjour la Switch ! éàç")
        agent.ajouter_message(d / "in", "pc", "Deuxième")
        msgs = agent.lire_messages(d / "in", d / "archive")
        self.assertEqual([m["texte"] for m in msgs], ["Bonjour la Switch ! éàç", "Deuxième"])
        self.assertEqual(agent.lire_messages(d / "in"), [])          # deja lus
        self.assertEqual(len(list((d / "archive").glob("*.json"))), 2)

    def test_semaine(self):
        auj = dt.date(2026, 11, 25)
        temps = "2026-11-24;Mario Kart 8 Deluxe;90\n2026-11-20;Zelda;45\n2026-11-25;Mario Kart 8 Deluxe;30\n2026-11-10;Vieux;999\nabime\n"
        sante = "date;source;batterie;etat\n2026-11-24T10:00:00;linux;37;Charging\n2026-11-23T10:00:00;linux;12;Discharging\n2026-10-01T10:00:00;linux;1;x\n"
        r = agent.resume_semaine(sante, temps, auj)
        self.assertEqual(r["minutes_total"], 165)
        self.assertEqual(r["jeux"][0], {"titre": "Mario Kart 8 Deluxe", "minutes": 120})
        self.assertEqual(r["batterie_min"], 12)
        self.assertIn("2 h 45", r["texte"])
        vide = agent.resume_semaine("", "", auj)
        self.assertEqual(vide["minutes_total"], 0)
        self.assertIsNone(vide["batterie_min"])


class Serveur(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.base = pathlib.Path(tempfile.mkdtemp())
        cls.cfg = {"jeton": "j" * 40, "port": 0}
        cls.srv = agent.creer_serveur(cls.cfg, cls.base, "127.0.0.1")
        cls.port = cls.srv.server_address[1]
        threading.Thread(target=cls.srv.serve_forever, daemon=True).start()

    @classmethod
    def tearDownClass(cls):
        cls.srv.shutdown()
        cls.srv.server_close()

    def req(self, methode, chemin, corps=None, jeton=True):
        c = http.client.HTTPConnection("127.0.0.1", self.port, timeout=10)
        h = {"X-HNP-Jeton": self.cfg["jeton"]} if jeton else {}
        if corps is not None:
            h["Content-Length"] = str(len(corps))
        c.request(methode, chemin, body=corps, headers=h)
        r = c.getresponse()
        data = r.read()
        c.close()
        return r.status, (json.loads(data) if data else None)

    def test_ping_sans_jeton(self):
        self.assertEqual(self.req("GET", "/ping", jeton=False)[0], 200)

    def test_jeton_obligatoire(self):
        self.assertEqual(self.req("GET", "/boite", jeton=False)[0], 401)

    def test_message_switch_vers_pc(self):
        st, r = self.req("POST", "/boite", json.dumps({"texte": "Salut le PC"}).encode())
        self.assertEqual(st, 201)
        f = self.base / "boite" / "de_switch" / r["fichier"]
        self.assertEqual(json.loads(f.read_text(encoding="utf-8"))["texte"], "Salut le PC")
        self.assertEqual(self.req("POST", "/boite", b"pas du json")[0], 400)

    def test_message_pc_vers_switch(self):
        agent.ajouter_message(self.base / "boite" / "vers_switch", "pc", "Coucou la Switch")
        st, r = self.req("GET", "/boite")
        self.assertEqual(st, 200)
        self.assertIn("Coucou la Switch", [m["texte"] for m in r["messages"]])

    def test_depot(self):
        st, r = self.req("PUT", "/depot/captures/ecran%201.jpg", b"\xff\xd8\xff" + b"0" * 1000)
        self.assertEqual(st, 201)
        self.assertTrue((self.base / r["chemin"]).exists())
        self.assertTrue(r["chemin"].endswith("ecran 1.jpg"))
        self.assertEqual(self.req("PUT", "/depot/captures/..%2F..%2Fx", b"1")[0], 400)
        self.assertEqual(self.req("PUT", "/depot/systeme/x.bin", b"1")[0], 400)

    def test_semaine_et_reveil(self):
        self.assertEqual(self.req("GET", "/semaine")[0], 200)
        st, r = self.req("GET", "/reveil")
        self.assertEqual(st, 200)
        self.assertRegex(r["mac"], r"^([0-9A-F]{2}:){5}[0-9A-F]{2}$")

    def test_ouvrir_liste_fermee(self):
        ouvertes = []
        self.srv.RequestHandlerClass.ouvrir = staticmethod(ouvertes.append)  # rien ne s'ouvre vraiment
        st, r = self.req("POST", "/ouvrir", json.dumps({"service": "netflix"}).encode())
        self.assertEqual((st, r["service"]), (200, "netflix"))
        self.assertEqual(ouvertes, ["https://www.netflix.com/browse"])
        for refuse in ({"service": "https://exemple.com"}, {"service": "cmd.exe"}, {"service": 3}, {}):
            self.assertEqual(self.req("POST", "/ouvrir", json.dumps(refuse).encode())[0], 400)
        self.assertEqual(self.req("POST", "/ouvrir", b"x", jeton=False)[0], 401)
        self.assertEqual(len(ouvertes), 1)
        # la liste du PC et celle de la Switch (hnp/services.hpp) doivent être identiques
        import re, pathlib
        hpp = (pathlib.Path(__file__).resolve().parent.parent / "projects/homenxplus/include/hnp/services.hpp").read_text(encoding="utf-8")
        switch_ids = set(re.findall(r'\{"([a-z0-9]+)", "', hpp))
        self.assertEqual(set(agent.SERVICES), switch_ids)
        self.assertTrue(all(u.startswith("https://") for u in agent.SERVICES.values()))


if __name__ == "__main__":
    unittest.main()
