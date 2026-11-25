# Génère promo/boutique.html depuis boutique/catalogue.json (une seule source pour la page et l'installeur).
#   py -I boutique/generer_page.py
import json, pathlib

racine = pathlib.Path(__file__).resolve().parent.parent
cat = json.loads((racine / 'boutique' / 'catalogue.json').read_text(encoding='utf-8'))
data = json.dumps(cat, ensure_ascii=False).replace('<', '\\u003c')

page = r'''<!doctype html>
<html lang="fr">
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">
<title>Boutique Switch</title>
<meta name="claude:viewport-fit-sides" content="cover">
<link rel="preconnect" href="https://fonts.googleapis.com">
<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
<link rel="stylesheet" href="https://fonts.googleapis.com/css2?family=Unbounded:wght@600;700&family=Manrope:wght@400;600;700&family=JetBrains+Mono:wght@500;600&display=swap">
<style>
/* Mise en page : une boutique ; filtres et panier collants en haut, puis une grille de fiches par rayon, puis « comment ça marche ». */
:root {
  --bg: #f4f5f8; --panel: #ffffff; --ink: #151821; --muted: #5c6374; --line: #dde1e9; --chip: #edf0f5;
  --accent: #e2402f; --accent-ink: #ffffff; --ok: #12875d; --ok-soft: #e1f3eb; --warn: #a65f00; --warn-soft: #fcefd9;
  --display: "Unbounded", "Arial Black", system-ui, sans-serif; --body: "Manrope", "Segoe UI", system-ui, sans-serif; --mono: "JetBrains Mono", Consolas, monospace;
}
@media (prefers-color-scheme: dark) { :root:not([data-theme="light"]) {
  --bg: #0d0f15; --panel: #161a24; --ink: #eef1f7; --muted: #9aa2b4; --line: #282d3a; --chip: #20242f;
  --accent: #ff5a4a; --accent-ink: #14161c; --ok: #3fd39a; --ok-soft: #12301f; --warn: #ffb64a; --warn-soft: #33250f; color-scheme: dark } }
:root[data-theme="dark"] {
  --bg: #0d0f15; --panel: #161a24; --ink: #eef1f7; --muted: #9aa2b4; --line: #282d3a; --chip: #20242f;
  --accent: #ff5a4a; --accent-ink: #14161c; --ok: #3fd39a; --ok-soft: #12301f; --warn: #ffb64a; --warn-soft: #33250f; color-scheme: dark }
* { box-sizing: border-box; }
body { margin: 0; background: var(--bg); color: var(--ink); font: 15px/1.5 var(--body);
  padding-left: max(16px, env(safe-area-inset-left, 0px)); padding-right: max(16px, env(safe-area-inset-right, 0px)); padding-block: 0 64px; }
body::before { content: ""; position: fixed; inset: 0 0 auto; height: env(safe-area-inset-top, 0px); z-index: 10; background: var(--bg); }
.wrap { max-width: 1040px; margin: 0 auto; display: grid; gap: 22px; padding-top: 26px; }
header { display: grid; gap: 8px; }
.eyebrow { font: 600 12px var(--mono); letter-spacing: .12em; text-transform: uppercase; color: var(--accent); }
h1 { font: 700 clamp(28px, 5.4vw, 44px)/1.05 var(--display); margin: 0; text-wrap: balance; }
.lead { color: var(--muted); margin: 0; max-width: 64ch; }
.bar { position: sticky; top: env(safe-area-inset-top, 0px); z-index: 5; background: var(--bg); padding: 10px 0; border-bottom: 1px solid var(--line); display: grid; gap: 10px; }
.row { display: flex; flex-wrap: wrap; gap: 8px 12px; align-items: center; }
.search { flex: 1 1 220px; min-width: 0; font: 15px var(--body); color: var(--ink); background: var(--panel); border: 1px solid var(--line); border-radius: 10px; padding: 9px 12px; }
.rayons { display: flex; flex-wrap: wrap; gap: 6px; }
.rayons button { font: 600 13px var(--body); color: var(--muted); background: var(--panel); border: 1px solid var(--line); border-radius: 99px; padding: 6px 12px; cursor: pointer; }
.rayons button[aria-pressed="true"] { background: var(--ink); color: var(--bg); border-color: var(--ink); }
.cart { display: flex; flex-wrap: wrap; gap: 8px 14px; align-items: center; justify-content: space-between; background: var(--panel); border: 1px solid var(--line); border-radius: 12px; padding: 10px 14px; }
.cart b { font-family: var(--display); font-weight: 600; }
.cart .sum { font: 600 13px var(--mono); color: var(--muted); font-variant-numeric: tabular-nums; }
.btns { display: flex; flex-wrap: wrap; gap: 8px; }
.btn { font: 700 14px var(--body); background: var(--accent); color: var(--accent-ink); border: 0; border-radius: 10px; padding: 9px 14px; cursor: pointer; }
.btn.ghost { background: transparent; color: var(--ink); border: 1px solid var(--line); }
.btn:disabled { opacity: .45; cursor: not-allowed; }
.save { font-size: 13px; color: var(--muted); }
.save.err { color: var(--accent); }
section.rayon { display: grid; gap: 10px; }
h2 { font: 600 19px var(--display); margin: 6px 0 0; }
.rtext { margin: 0; color: var(--muted); font-size: 14px; max-width: 70ch; }
.grid { display: grid; grid-template-columns: repeat(auto-fill, minmax(240px, 1fr)); gap: 12px; }
label.card { position: relative; display: grid; grid-template-rows: auto 1fr auto; gap: 6px; background: var(--panel); border: 1.5px solid var(--line); border-radius: 14px; padding: 14px; cursor: pointer; min-width: 0; transition: border-color .12s, transform .12s; }
label.card:hover { transform: translateY(-1px); border-color: var(--muted); }
label.card.on { border-color: var(--ok); background: linear-gradient(0deg, var(--ok-soft), var(--panel) 70%); }
label.card input { position: absolute; top: 14px; right: 14px; width: 20px; height: 20px; accent-color: var(--ok); }
.cname { font-weight: 700; font-size: 16px; padding-right: 30px; }
.ctext { color: var(--muted); font-size: 13.5px; }
.badges { display: flex; flex-wrap: wrap; gap: 5px; }
.badge { font: 600 10.5px var(--mono); letter-spacing: .05em; text-transform: uppercase; border-radius: 99px; padding: 2px 8px; background: var(--chip); color: var(--muted); }
.badge.reco { background: var(--accent); color: var(--accent-ink); }
.badge.mod { background: var(--warn-soft); color: var(--warn); }
label.card:focus-within { outline: 2px solid var(--accent); outline-offset: 2px; }
.how { background: var(--panel); border: 1px solid var(--line); border-radius: 14px; padding: 16px 18px; display: grid; gap: 8px; }
.how ol { margin: 0; padding-left: 22px; display: grid; gap: 4px; }
code { font: .88em var(--mono); background: var(--chip); border: 1px solid var(--line); border-radius: 5px; padding: 1px 6px; overflow-wrap: anywhere; }
.foot { color: var(--muted); font-size: 13px; border-top: 1px solid var(--line); padding-top: 12px; }
button:focus-visible, input:focus-visible { outline: 2px solid var(--accent); outline-offset: 2px; }
@media (prefers-reduced-motion: reduce) { label.card { transition: none; } label.card:hover { transform: none; } }
</style>

<div class="wrap">
  <header>
    <div class="eyebrow">Boutique HomeNX+ · applis libres pour Switch modifiée</div>
    <h1>Coche, et c'est installé.</h1>
    <p class="lead">Choisis tes applis comme dans un magasin. Ta liste s'enregistre ; ensuite, carte SD dans le PC, l'installeur télécharge chaque appli depuis sa page officielle et la range au bon endroit.</p>
  </header>

  <div class="bar">
    <div class="row">
      <input class="search" id="q" type="search" placeholder="Chercher une appli (films, manettes, sauvegarde…)" aria-label="Chercher une appli">
      <div class="rayons" id="rayons" role="group" aria-label="Rayons"></div>
    </div>
    <div class="cart" aria-live="polite">
      <div><b id="nb">Panier vide</b> <span class="sum" id="sum"></span></div>
      <div class="btns">
        <button type="button" class="btn ghost" id="pack">Pack débutant</button>
        <button type="button" class="btn ghost" id="vider">Vider</button>
        <button type="button" class="btn" id="export">Télécharger ma liste</button>
      </div>
      <div class="save" id="save"></div>
    </div>
  </div>

  <div id="rayonsListe"></div>

  <section class="how">
    <h2>Comment ça marche</h2>
    <ol>
      <li>Coche tes applis. Le <b>Pack débutant</b> coche les essentielles.</li>
      <li><b>Télécharge ta liste</b> (<code>ma_liste_switch.json</code>), ou dis simplement à Claude « ma liste est prête ».</li>
      <li>Carte SD de la Switch dans le PC, puis : <code>.\boutique\installer_boutique.ps1 -Liste ma_liste_switch.json</code></li>
      <li>Remets la carte dans la Switch. Les modules marqués « en arrière-plan » s'allument dans Ultrahand &gt; Sysmodules.</li>
    </ol>
    <p class="rtext">L'installeur ne touche jamais au menu HOME, à Hekate ni aux réglages d'Atmosphère, et n'écrase rien de ce qui est déjà sur la carte.</p>
  </section>

  <p class="foot">Toutes ces applis sont libres et viennent de leur page GitHub officielle. Aucune clé, aucun firmware, aucun jeu, aucun installeur de jeux n'est proposé. Il faut une Switch déjà modifiée (Atmosphère).</p>
</div>

<script>
const CAT = __DATA__;
const byId = Object.fromEntries(CAT.applis.map(a => [a.id, a]));
let pick = new Set(), rayon = 'tout', q = '', db = null, docRef = null, writing = false, pending = false, timer = 0, dl = null;
try { const s = JSON.parse(localStorage.getItem('hnp-boutique') || '[]'); if (Array.isArray(s)) s.forEach(id => byId[id] && pick.add(id)); } catch (e) {}

const norm = s => (s || '').normalize('NFD').replace(/[̀-ͯ]/g, '').toLowerCase();
const rayonsEl = document.getElementById('rayons');
for (const [id, nom] of [['tout', 'Tout'], ...CAT.categories.map(c => [c.id, c.nom])]) {
  const b = document.createElement('button'); b.type = 'button'; b.textContent = nom; b.dataset.r = id;
  b.onclick = () => { rayon = id; render(); }; rayonsEl.append(b);
}
document.getElementById('q').addEventListener('input', e => { q = norm(e.target.value.trim()); render(); });

function render() {
  for (const b of rayonsEl.children) b.setAttribute('aria-pressed', String(b.dataset.r === rayon));
  const root = document.getElementById('rayonsListe'); root.textContent = '';
  for (const c of CAT.categories) {
    if (rayon !== 'tout' && rayon !== c.id) continue;
    const apps = CAT.applis.filter(a => a.cat === c.id && (!q || norm(a.nom + ' ' + a.texte).includes(q)));
    if (!apps.length) continue;
    const sec = document.createElement('section'); sec.className = 'rayon';
    const h = document.createElement('h2'); h.textContent = c.nom;
    const p = document.createElement('p'); p.className = 'rtext'; p.textContent = c.texte;
    const grid = document.createElement('div'); grid.className = 'grid';
    for (const a of apps) {
      const lab = document.createElement('label'); lab.className = 'card' + (pick.has(a.id) ? ' on' : '');
      const cb = document.createElement('input'); cb.type = 'checkbox'; cb.id = 'app-' + a.id; cb.checked = pick.has(a.id);
      cb.onchange = () => { cb.checked ? pick.add(a.id) : pick.delete(a.id); lab.classList.toggle('on', cb.checked); summary(); queueSave(); };
      const n = document.createElement('div'); n.className = 'cname'; n.textContent = a.nom;
      const t = document.createElement('div'); t.className = 'ctext'; t.textContent = a.texte;
      const bd = document.createElement('div'); bd.className = 'badges';
      const add = (txt, cl) => { const s = document.createElement('span'); s.className = 'badge' + (cl ? ' ' + cl : ''); s.textContent = txt; bd.append(s); };
      if (a.recommande) add('Recommandé', 'reco');
      if (a.module) add('En arrière-plan', 'mod');
      add(a.depot === 'local' ? 'HomeNX+' : a.licence);
      lab.append(cb, n, t, bd); grid.append(lab);
    }
    sec.append(h, p, grid); root.append(sec);
  }
  if (!root.children.length) { const p = document.createElement('p'); p.className = 'rtext'; p.textContent = 'Aucune appli ne correspond.'; root.append(p); }
  summary();
}
function summary() {
  const n = pick.size;
  document.getElementById('nb').textContent = n ? n + (n > 1 ? ' applis' : ' appli') + ' dans ton panier' : 'Panier vide';
  const mods = [...pick].filter(id => byId[id].module).length;
  document.getElementById('sum').textContent = n ? (mods ? mods + ' en arrière-plan' : '') : '';
  document.getElementById('export').disabled = !n;
  document.getElementById('vider').disabled = !n;
}
function status(t, err) { const s = document.getElementById('save'); s.textContent = t; s.className = 'save' + (err ? ' err' : ''); }
function liste() { return { boutique: 'HomeNX+', catalogue: CAT.version, date: new Date().toISOString(), applis: [...pick] }; }
function queueSave() {
  try { localStorage.setItem('hnp-boutique', JSON.stringify([...pick])); } catch (e) {}
  clearTimeout(timer); timer = setTimeout(flush, 600);
  status(docRef ? 'Enregistrement…' : 'Panier gardé dans ce navigateur');
}
async function flush() {
  if (!docRef) return;
  if (writing) { pending = true; return; }
  writing = true;
  try { do { pending = false; await docRef.set(liste()); } while (pending); status('Panier enregistré : Claude peut le lire.'); }
  catch (e) { status(e && e.code === 'invalid_argument' ? 'Tu peux voir la boutique mais pas enregistrer de panier : télécharge ta liste.' : 'Panier pas enregistré pour l\'instant.', true); }
  finally { writing = false; }
}
document.getElementById('pack').onclick = () => { CAT.applis.filter(a => a.recommande).forEach(a => pick.add(a.id)); render(); queueSave(); };
document.getElementById('vider').onclick = () => { pick.clear(); render(); queueSave(); };
document.getElementById('export').onclick = async () => {
  const text = JSON.stringify(liste(), null, 1);
  if (dl) { try { await dl.save({ filename: 'ma_liste_switch.json', data: text }); status('Liste téléchargée.'); } catch (e) { status('Téléchargement refusé ou annulé.', true); } return; }
  try {
    const a = document.createElement('a'); a.href = URL.createObjectURL(new Blob([text], { type: 'application/json' }));
    a.download = 'ma_liste_switch.json'; a.click(); setTimeout(() => URL.revokeObjectURL(a.href), 2000); status('Liste téléchargée.');
  } catch (e) { status('Téléchargement impossible ici.', true); }
};
render();
status(pick.size ? 'Panier gardé dans ce navigateur' : '');

(async () => {
  if (!window.claude) return;
  dl = await window.claude.use('downloads');
  db = await window.claude.use('db');
  const user = await window.claude.use('user');
  const id = user ? await user.id() : null;
  if (!db || !id) return;
  docRef = db.doc('paniers/' + id);
  const t = setTimeout(() => status('Ton panier enregistré met du temps à arriver.', true), 10000);
  try {
    const snap = await docRef.get(); clearTimeout(t);
    const v = snap && snap.exists ? snap.data() : null;
    if (v && Array.isArray(v.applis)) { v.applis.forEach(x => byId[x] && pick.add(x)); render(); }
    status(pick.size ? 'Panier enregistré : Claude peut le lire.' : 'Coche tes applis : ton panier s\'enregistre tout seul.');
    if (pick.size && !(v && Array.isArray(v.applis) && v.applis.length === pick.size)) flush();
  } catch (e) { clearTimeout(t); status('Impossible de lire ton panier enregistré.', true); }
})();
</script>
</html>
'''
(racine / 'promo' / 'boutique.html').write_text(page.replace('__DATA__', data), encoding='utf-8', newline='\n')
print('promo/boutique.html :', len(cat['applis']), 'applis,', len(cat['categories']), 'rayons')
