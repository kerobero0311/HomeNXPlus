# HomeNX+ : prépare le paquet de diffusion (à décompresser à la racine d'une carte SD).
#   .\outils\creer_release.ps1            # version lue dans le code (CardsHnp.cpp)
# Contenu : les .nro (version de test), l'overlay, les scripts Linux, la config par défaut,
# le compagnon PC, le guide hors ligne, LICENSE, CREDITS et les empreintes SHA-256.
# Jamais dedans : clé API, pc_agent.ini (adresse et jeton du PC), journaux, données personnelles.
$ErrorActionPreference = 'Stop'
$Racine = Split-Path -Parent $PSScriptRoot
$m = Select-String -Path "$Racine\projects\cards\src\CardsHnp.cpp" -Pattern 'kVersion\s+=\s+"HomeNX\+ ([0-9.]+)"' | Select-Object -First 1
if (-not $m) { throw 'Version introuvable dans CardsHnp.cpp' }
$Version = $m.Matches[0].Groups[1].Value
$Nom = "HomeNXPlus-$Version"
$Dist = Join-Path $Racine 'dist'
$Dossier = Join-Path $Dist $Nom
Remove-Item -Recurse -Force $Dossier -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force $Dossier | Out-Null

$bin = Join-Path $Racine 'build\homebrew\cross\aarch64\release'
$plan = @(
    @("$bin\HomeNX+.nro",                       'SD\switch\HomeNXPlus\HomeNXPlus.nro'),
    @("$bin\ClaudeNX.nro",                      'SD\switch\ClaudeNX\ClaudeNX.nro'),
    @("$Racine\projects\overlay\HomeNXPlus.ovl",'SD\switch\.overlays\HomeNXPlus.ovl'),
    @("$Racine\assets\cacert.pem",              'SD\config\homenxplus\cacert.pem'),
    @("$Racine\assets\config.ini",              'SD\config\homenxplus\config.ini'),
    @("$Racine\LICENSE",                        'LICENSE'),
    @("$Racine\CREDITS.md",                     'CREDITS.md'),
    @("$Racine\CHANGELOG.md",                   'CHANGELOG.md')
)
foreach ($s in 'secours_menu_home', 'rapport_sante', 'boite_aux_lettres', 'installer', 'filmer_ecran') {
    $plan += ,@("$Racine\linux\$s.sh", "SD\switchroot\outils\homenxplus\$s.sh")
}
foreach ($f in 'agent.py', 'test_agent.py', 'lancer_pc_agent.ps1', 'config_switch.py') {
    if (Test-Path "$Racine\pc_agent\$f") { $plan += ,@("$Racine\pc_agent\$f", "PC\pc_agent\$f") }
}
$plan += ,@("$Racine\secours_menu_home.ps1", 'PC\secours_menu_home.ps1')
foreach ($f in 'index.html', 'guide.html', 'Guide_Switch_HomeNXPlus.pdf', 'trailer.html', 'tournage.html') {
    $plan += ,@("$Racine\promo\$f", "Guide\$f")
}

foreach ($p in $plan) {
    if (-not (Test-Path $p[0])) { throw "Fichier manquant : $($p[0]) (compile d'abord : .\build.ps1 -Variant homebrew)" }
    $dst = Join-Path $Dossier $p[1]
    New-Item -ItemType Directory -Force (Split-Path $dst) | Out-Null
    if ($p[1] -like '*.sh') {  # fins de ligne Linux
        [IO.File]::WriteAllText($dst, ([IO.File]::ReadAllText($p[0]) -replace "`r`n", "`n"), (New-Object Text.UTF8Encoding($false)))
    } else { Copy-Item $p[0] $dst }
}

# Garde-fou : aucune donnée personnelle
$interdits = 'api_key', 'pc_agent.ini', 'agent.json', 'discord.json', 'prod.keys', 'title.keys'
$trouves = Get-ChildItem $Dossier -Recurse -File | Where-Object { $n = $_.Name; $interdits | Where-Object { $n -like "*$_*" } }
if ($trouves) { throw "Fichier interdit dans le paquet : $($trouves.Name -join ', ')" }
$texte = Get-ChildItem $Dossier -Recurse -File -Include *.html, *.md, *.ini, *.py, *.ps1, *.sh | Select-String -Pattern 'sk-ant-api0[0-9]-[A-Za-z0-9_-]{20,}', '10\.204\.', '14:AC:60' -List
if ($texte) { throw "Donnée personnelle trouvée : $($texte.Path -join ', ')" }

$lisez = @"
HomeNX+ $Version : menu HOME et outils pour Nintendo Switch modifiée (Atmosphère)
=============================================================================

FRANÇAIS
- Copie le contenu du dossier SD à la racine de ta carte SD.
- Lance HomeNX+ depuis le menu homebrew (en maintenant R sur un jeu pour avoir toute la mémoire).
- Claude (facultatif) : mets ta clé API seule dans sd:/config/homenxplus/api_key.txt ; plafond mensuel dans config.ini.
- Compagnon PC (facultatif) : dossier PC\pc_agent, à lancer avec lancer_pc_agent.ps1 (Python 3).
- Mode d'emploi complet : Guide\index.html (s'ouvre hors ligne) ou Guide\Guide_Switch_HomeNXPlus.pdf.
- Cette version s'utilise comme une appli : elle ne remplace pas le menu HOME de la console.

ENGLISH
- Copy the content of the SD folder to the root of your SD card.
- Start HomeNX+ from the homebrew menu (hold R while starting a game for full memory).
- Claude (optional): put your own Anthropic API key alone in sd:/config/homenxplus/api_key.txt.
- The guide (French) is in Guide\index.html.

Aucune clé, aucun firmware ni aucun jeu n'est fourni. No keys, firmware or games are included.
Licence GPL-3.0 (voir LICENSE). D'après HomeNX (ad3333m) et SwitchU (PoloNX) : voir CREDITS.md.
"@
[IO.File]::WriteAllText((Join-Path $Dossier 'LISEZMOI.txt'), $lisez, (New-Object Text.UTF8Encoding($true)))

# Empreintes SHA-256
$sums = Get-ChildItem $Dossier -Recurse -File | Sort-Object FullName | ForEach-Object {
    '{0}  {1}' -f (Get-FileHash $_.FullName -Algorithm SHA256).Hash.ToLower(), $_.FullName.Substring($Dossier.Length + 1).Replace('\', '/')
}
[IO.File]::WriteAllLines((Join-Path $Dossier 'SHA256SUMS.txt'), $sums)

$Zip = Join-Path $Dist "$Nom.zip"
Remove-Item -Force $Zip -ErrorAction SilentlyContinue
Compress-Archive -Path "$Dossier\*" -DestinationPath $Zip -CompressionLevel Optimal
$taille = (Get-Item $Zip).Length / 1MB
Write-Host ("Paquet prêt : {0} ({1:N1} Mo, {2} fichiers)" -f $Zip, $taille, $sums.Count) -ForegroundColor Green
Write-Host ("SHA-256 du zip : " + (Get-FileHash $Zip -Algorithm SHA256).Hash.ToLower())
