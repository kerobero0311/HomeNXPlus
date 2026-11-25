# HomeNX+ : TOUT EN UNE PASSE, carte SD de la Switch dans l'Ally.
#   1. trouve la carte (« SWITCH SD »), quelle que soit sa lettre
#   2. vérifie la santé de la console (lecture seule)
#   3. compile HomeNX+ si le code a changé depuis la dernière compilation
#   4. installe HomeNX+ et le dossier de documents (seuls dossiers autorisés, .tmp puis renommage, SHA-256)
#   5. récupère sur le PC : journaux et rapports de plantage, nouvelles captures, rushes du trailer
#   6. affiche un bilan, et le garde dans rapports\bilan_<date>.txt
# Ne touche jamais au menu HOME réel, à Hekate, à emuMMC, aux clés ni à la clé API. N'éjecte pas la carte.
[CmdletBinding()]
param([switch]$SansCompilation, [switch]$Simulation)
$ErrorActionPreference = 'Stop'
$Racine = Split-Path -Parent $PSScriptRoot
$bilan = [System.Collections.Generic.List[string]]::new()
function Note([string]$t, [string]$c = 'Gray') { Write-Host $t -ForegroundColor $c; $bilan.Add($t) }
function Etape([string]$t) { Write-Host ''; Note "== $t ==" 'Cyan' }

# ---------------------------------------------------------------- 1. la carte
Etape 'Carte SD'
$v = Get-Volume | Where-Object { $_.FileSystemLabel -eq 'SWITCH SD' -and $_.DriveLetter } | Select-Object -First 1
if (-not $v) { Note "Carte « SWITCH SD » introuvable : mets-la dans le lecteur de l'Ally et relance." 'Red'; exit 1 }
$L = "$($v.DriveLetter):"
Note ("Carte trouvée sur {0} : {1:N1} Go libres sur {2:N0} Go" -f $L, ($v.SizeRemaining / 1GB), ($v.Size / 1GB)) 'Green'

# ---------------------------------------------------------------- 2. santé (lecture seule)
Etape 'Santé de la console'
$alertes = 0
foreach ($chemin in 'atmosphere\package3', 'bootloader\hekate_ipl.ini', 'emuMMC\emummc.ini', 'atmosphere\contents\0100000000001000\exefs.nsp') {
    if (Test-Path "$L\$chemin") { Note "OK   $chemin" } else { Note "MANQUE $chemin" 'Red'; $alertes++ }
}
$sp = "$L\config\sys-patch\config.ini"
if (Test-Path $sp) {
    $cfg = Get-Content $sp -Raw
    if ($cfg -match 'patch_emummc\s*=\s*0') { Note "ALERTE sys-patch : patch_emummc=0 (les jeux ne se lanceront plus). Remets-le à 1 via l'overlay sys-patch." 'Red'; $alertes++ }
    else { Note 'OK   sys-patch (patch_emummc actif)' }
}
if ($v.SizeRemaining -lt 5GB) { Note ("ALERTE place : moins de 5 Go libres ({0:N1} Go)" -f ($v.SizeRemaining / 1GB)) 'Yellow'; $alertes++ }
$crashs = @(Get-ChildItem "$L\atmosphere\crash_reports" -File -Filter *.log -ErrorAction SilentlyContinue | Where-Object { $_.LastWriteTime -gt (Get-Date).AddDays(-7) })
Note ("Rapports de plantage des 7 derniers jours : {0}" -f $crashs.Count) ($(if ($crashs.Count) { 'Yellow' } else { 'Gray' }))
$starts = Get-Content "$L\config\homenxplus\menu\hnp_starts" -ErrorAction SilentlyContinue
if ($starts -and [int]$starts -ge 3) { Note "HomeNX+ : $starts démarrages ratés de suite (mode sans échec). Le journal : config\homenxplus\cards\start.log" 'Yellow' }

# ---------------------------------------------------------------- 3. compilation si besoin
Etape 'Compilation'
$nro = Join-Path $Racine 'build\homebrew\cross\aarch64\release\HomeNX+.nro'
$recent = Get-ChildItem "$Racine\projects", "$Racine\lib\nxui" -Recurse -File -Include *.cpp, *.hpp, *.h, *.lua -ErrorAction SilentlyContinue |
    Sort-Object LastWriteTime -Descending | Select-Object -First 1
$aCompiler = -not (Test-Path $nro) -or ($recent -and $recent.LastWriteTime -gt (Get-Item $nro).LastWriteTime) -or
             ((Get-Item "$Racine\xmake.lua").LastWriteTime -gt (Get-Item $nro -ErrorAction SilentlyContinue).LastWriteTime)
if ($SansCompilation) { Note 'Compilation sautée (demandé).' }
elseif ($aCompiler) {
    Note 'Le code a changé : compilation de HomeNX+ (1 à 2 minutes)...' 'Yellow'
    & "$Racine\build.ps1" -Variant homebrew *> (Join-Path $env:TEMP 'hnp_build.log')
    if (-not $?) { Note "Échec de la compilation : voir $env:TEMP\hnp_build.log" 'Red'; exit 1 }
    Note 'Compilation OK' 'Green'
} else { Note 'HomeNX+ est déjà à jour.' }

# ---------------------------------------------------------------- 4. installation
Etape 'Installation sur la carte'
if ($Simulation) { & "$Racine\outils\installer_sur_sd.ps1" -Lecteur $v.DriveLetter -Simulation | ForEach-Object { Note "$_" } }
else { & "$Racine\outils\installer_sur_sd.ps1" -Lecteur $v.DriveLetter | ForEach-Object { Note "$_" } }
$docs = "$L\homenxplus\docs"
$copies = 0
foreach ($f in 'index.html', 'guide.html', 'Guide_Switch_HomeNXPlus.pdf', 'trailer.html', 'ludotheque.html', 'tournage.html') {
    $src = Join-Path "$Racine\promo" $f
    if (-not (Test-Path $src)) { continue }
    $dst = Join-Path $docs $f
    if ((Test-Path $dst) -and (Get-FileHash $src).Hash -eq (Get-FileHash $dst).Hash) { continue }
    if (-not $Simulation) {
        New-Item -ItemType Directory -Force $docs | Out-Null
        Copy-Item $src "$dst.tmp" -Force; Move-Item -Force "$dst.tmp" $dst
    }
    $copies++
}
Note ("Documents (homenxplus/docs) : {0} fichier(s) mis à jour" -f $copies)
if ($copies -and -not $Simulation) { Add-Content -Encoding utf8 "$L\VERSIONS.txt" ((Get-Date -Format 'yyyy-MM-dd HH:mm') + " : homenxplus/docs mis à jour ($copies fichier(s), tout_en_une_passe.ps1)") }

# ---------------------------------------------------------------- 5. récupération sur le PC (lecture seule sur la carte)
Etape 'Récupération sur le PC'
$jour = Get-Date -Format 'yyyy-MM-dd'
$sauve = Join-Path ([Environment]::GetFolderPath('MyDocuments')) "Switch_Sauvegardes\$jour"
# journaux et rapports de plantage
$jdst = Join-Path $sauve 'journaux'
$nj = 0
foreach ($s in @(@("$L\atmosphere\crash_reports", '*.log'), @("$L\atmosphere\fatal_reports", '*'), @("$L\config\SwitchU", '*.log'),
                 @("$L\config\homenxplus\menu", 'log*.txt'), @("$L\config\homenxplus\cards", '*.log'))) {
    if (-not (Test-Path $s[0])) { continue }
    foreach ($f in Get-ChildItem $s[0] -File -Filter $s[1] -ErrorAction SilentlyContinue | Where-Object { $_.LastWriteTime -gt (Get-Date).AddDays(-7) }) {
        $d = Join-Path $jdst (Split-Path $s[0] -Leaf)
        if (-not $Simulation) { New-Item -ItemType Directory -Force $d | Out-Null; Copy-Item $f.FullName (Join-Path $d $f.Name) -Force }
        $nj++
    }
}
Note "Journaux des 7 derniers jours : $nj fichier(s) -> $jdst"
# nouvelles captures de l'Album (photos et vidéos), une seule fois chacune
$album = "$L\emuMMC\RAW1\Nintendo\Album"
$cdst = Join-Path ([Environment]::GetFolderPath('MyDocuments')) 'Switch_Sauvegardes\captures'
$nc = 0
if (Test-Path $album) {
    foreach ($f in Get-ChildItem $album -Recurse -File -Include *.jpg, *.mp4) {
        $d = Join-Path $cdst ($f.LastWriteTime.ToString('yyyy-MM'))
        $t = Join-Path $d $f.Name
        if (Test-Path $t) { continue }
        if (-not $Simulation) { New-Item -ItemType Directory -Force $d | Out-Null; Copy-Item $f.FullName $t }
        $nc++
    }
}
Note "Nouvelles captures de l'Album : $nc -> $cdst"
# rushes du trailer
if (-not $Simulation) { & "$Racine\outils\recuperer_tournage.ps1" -Carte $L | ForEach-Object { Note "$_" } }

# ---------------------------------------------------------------- 6. bilan
Etape 'Bilan'
if ($alertes) { Note "$alertes alerte(s) à regarder (en rouge ou jaune ci-dessus)." 'Yellow' } else { Note 'Tout va bien.' 'Green' }
Write-VolumeCache $v.DriveLetter
Note "Carte toujours montée sur $L (non éjectée). Tu peux la retirer quand tu veux."
$rapport = Join-Path $Racine ("rapports\bilan_" + (Get-Date -Format 'yyyy-MM-dd_HHmm') + '.txt')
New-Item -ItemType Directory -Force (Split-Path $rapport) | Out-Null
[IO.File]::WriteAllLines($rapport, $bilan, (New-Object Text.UTF8Encoding($true)))
Write-Host "Bilan gardé dans $rapport"
