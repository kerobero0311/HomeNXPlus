# Boutique HomeNX+ : installe sur la carte SD les applis choisies dans la boutique.
#   .\boutique\installer_boutique.ps1 -Liste ma_liste_switch.json
#   .\boutique\installer_boutique.ps1 -Ids switchwave,gmca,newpipe
# Chaque appli vient de sa page GitHub officielle (dernière version). Le contenu est vérifié
# avant la copie : seuls switch/, config/<appli>/ et atmosphere/contents/<module>/ sont acceptés
# (jamais le menu HOME, Hekate, emuMMC ni les réglages d'Atmosphère). Rien n'est écrasé sans
# -MettreAJour. Les modules en arrière-plan ne démarrent pas tout seuls sans -Demarrage :
# on les allume dans Ultrahand > Sysmodules.
[CmdletBinding()]
param(
    [string]$Liste = '',
    [string[]]$Ids = @(),
    [string]$Lecteur = '',
    [switch]$MettreAJour,
    [switch]$Demarrage,
    [switch]$Simulation
)
$ErrorActionPreference = 'Stop'
$Racine = Split-Path -Parent $PSScriptRoot
$cat = Get-Content (Join-Path $PSScriptRoot 'catalogue.json') -Raw -Encoding UTF8 | ConvertFrom-Json
$parId = @{}; foreach ($a in $cat.applis) { $parId[$a.id] = $a }

# --- la liste
if ($Liste) {
    $j = Get-Content $Liste -Raw -Encoding UTF8 | ConvertFrom-Json
    $Ids += @($j.applis)
}
$Ids = @($Ids | ForEach-Object { $_ -split ',' } | ForEach-Object { $_.Trim() } | Where-Object { $_ } | Select-Object -Unique)
if (-not $Ids) { Write-Host 'Liste vide : coche des applis dans la boutique, ou passe -Ids.' -ForegroundColor Yellow; exit 1 }
$inconnus = @($Ids | Where-Object { -not $parId.ContainsKey($_) })
if ($inconnus) { Write-Host "Inconnu(s) dans le catalogue, ignoré(s) : $($inconnus -join ', ')" -ForegroundColor Yellow }
$Ids = @($Ids | Where-Object { $parId.ContainsKey($_) })
# nx-ovlloader d'abord si un overlay est demandé
if (($Ids | Where-Object { $parId[$_].cat -eq 'enjeu' }) -and ($Ids -notcontains 'ovlloader')) { $Ids = @('ovlloader') + $Ids }

# --- la carte
if (-not $Lecteur) {
    $v = Get-Volume | Where-Object { $_.FileSystemLabel -eq 'SWITCH SD' -and $_.DriveLetter } | Select-Object -First 1
    if (-not $v) { $v = Get-Volume | Where-Object { $_.DriveType -eq 'Removable' -and $_.DriveLetter -and (Test-Path "$($_.DriveLetter):\atmosphere") } | Select-Object -First 1 }
    if (-not $v) { Write-Host "Carte SD de la Switch introuvable (dossier atmosphere absent)." -ForegroundColor Red; exit 1 }
    $Lecteur = $v.DriveLetter
}
$sd = "$($Lecteur.TrimEnd(':')):"
if (-not (Test-Path "$sd\atmosphere")) { Write-Host "$sd n'est pas une carte de Switch (pas de dossier atmosphere)." -ForegroundColor Red; exit 1 }
$homeAvant = (Get-FileHash "$sd\atmosphere\contents\0100000000001000\exefs.nsp" -ErrorAction SilentlyContinue).Hash
Write-Host "Carte : $sd ; $($Ids.Count) appli(s) : $($Ids -join ', ')" -ForegroundColor Cyan

$tmp = Join-Path $env:TEMP ("hnp_boutique_" + (Get-Date -Format 'yyyyMMdd_HHmmss'))
New-Item -ItemType Directory -Force $tmp | Out-Null
$etatFichier = "$sd\config\homenxplus\boutique.json"
$etat = @{}
if (Test-Path $etatFichier) { (Get-Content $etatFichier -Raw | ConvertFrom-Json).PSObject.Properties | ForEach-Object { $etat[$_.Name] = $_.Value } }
$bilan = @()
$headers = @{ 'User-Agent' = 'HomeNXPlus-boutique'; 'Accept' = 'application/vnd.github+json' }

function Test-Autorise([string]$rel) {
    $r = $rel.Replace('\', '/')
    if ($r -match '(^|/)\.\.(/|$)' -or $r -match '^/') { return $false }
    if ($r -match '^atmosphere/contents/0100000000001000/') { return $false }      # menu HOME : jamais
    if ($r -match '^switch/') { return $true }
    if ($r -match '^config/[^/]+/') { return $r -notmatch '^config/(sys-patch|homenxplus/api_key)' }
    if ($r -match '^config/[^/]+\.(ini|json|txt)$') { return $true }   # réglage posé directement dans config/
    if ($r -match '^atmosphere/contents/[0-9A-Fa-f]{16}/') { return $true }
    return $false
}

function Copier-Fichier([string]$src, [string]$rel, [string]$id) {
    $rel = $rel.Replace('/', '\')
    if (-not (Test-Autorise $rel)) { $script:bilan += "  refusé (hors des dossiers permis) : $rel"; return }
    if ($rel -match '\\flags\\boot2\.flag$' -and -not ($Demarrage -or $parId[$id].demarrage)) {
        $script:bilan += "  module non démarré automatiquement : à allumer dans Ultrahand > Sysmodules"; return
    }
    $dst = Join-Path $sd $rel
    if (Test-Path $dst) {
        if ((Get-FileHash $src).Hash -eq (Get-FileHash $dst).Hash) { return }
        if (-not $MettreAJour) { $script:bilan += "  déjà là, gardé : $rel"; return }
    }
    if ($Simulation) { $script:bilan += "  copierait : $rel"; return }
    New-Item -ItemType Directory -Force (Split-Path $dst) | Out-Null
    Copy-Item $src "$dst.tmp" -Force
    if ((Get-FileHash $src).Hash -ne (Get-FileHash "$dst.tmp").Hash) { Remove-Item "$dst.tmp"; throw "Copie différente : $rel" }
    Move-Item -Force "$dst.tmp" $dst
    $script:copies++
}

$total = 0
foreach ($id in $Ids) {
    $a = $parId[$id]
    $script:copies = 0
    Write-Host "== $($a.nom)" -ForegroundColor Cyan
    try {
        $dir = Join-Path $tmp $id; New-Item -ItemType Directory -Force $dir | Out-Null
        $version = ''
        if ($a.type -eq 'local') {
            $src = Join-Path $Racine "build\homebrew\cross\aarch64\release\$($a.fichier)"
            if (-not (Test-Path $src)) { throw "pas encore compilé ($($a.fichier))" }
            $nom = if ($a.id -eq 'homenxplus') { 'HomeNXPlus.nro' } else { $a.fichier }
            Copier-Fichier $src "switch/$($a.dossier)/$nom" $id
            $version = 'local'
        } else {
            $rel = Invoke-RestMethod -Headers $headers "https://api.github.com/repos/$($a.depot)/releases/latest"
            $asset = $rel.assets | Where-Object { $_.name -match $a.fichier } | Select-Object -First 1
            if (-not $asset) { throw "aucun fichier « $($a.fichier) » dans la version $($rel.tag_name)" }
            $version = $rel.tag_name
            if ($etat[$id] -eq $version -and -not $MettreAJour) { $bilan += "$($a.nom) $version : déjà à jour"; Write-Host '  déjà à jour'; continue }
            $f = Join-Path $dir $asset.name
            Invoke-WebRequest -Headers @{ 'User-Agent' = 'HomeNXPlus-boutique' } -Uri $asset.browser_download_url -OutFile $f -UseBasicParsing
            if ($asset.size -and (Get-Item $f).Length -ne $asset.size) { throw 'téléchargement incomplet' }
            switch ($a.type) {
                'nro' { Copier-Fichier $f "switch/$($a.dossier)/$($asset.name)" $id }
                'ovl' { Copier-Fichier $f "switch/.overlays/$($asset.name)" $id }
                'zip' {
                    $x = Join-Path $dir 'x'
                    Add-Type -AssemblyName System.IO.Compression.FileSystem
                    [IO.Compression.ZipFile]::ExtractToDirectory($f, $x)
                    # racine de la carte : directe, sous un seul dossier (sdout/…), ou appli « à plat »
                    $base = $x
                    $tops = @(Get-ChildItem $x -Force)
                    $carte = { param($d) @(Get-ChildItem $d -Directory -Force | Where-Object { $_.Name -in 'switch', 'atmosphere', 'config' }).Count -gt 0 }
                    if (-not (& $carte $x) -and $tops.Count -eq 1 -and $tops[0].PSIsContainer -and (& $carte $tops[0].FullName)) { $base = $tops[0].FullName }
                    if (& $carte $base) {
                        foreach ($g in Get-ChildItem $base -Recurse -File -Force) { Copier-Fichier $g.FullName $g.FullName.Substring($base.Length + 1) $id }
                    } else {
                        foreach ($g in Get-ChildItem $base -Recurse -File -Force) { Copier-Fichier $g.FullName ("switch/$($a.dossier)/" + $g.FullName.Substring($base.Length + 1)) $id }
                    }
                }
            }
        }
        if (-not $Simulation) { $etat[$id] = $version }
        $bilan += "$($a.nom) $version : $script:copies fichier(s) copié(s)"
        $total += $script:copies
        Write-Host "  $script:copies fichier(s) copié(s)" -ForegroundColor Green
    } catch {
        $bilan += "$($a.nom) : ÉCHEC ($($_.Exception.Message))"
        Write-Host "  échec : $($_.Exception.Message)" -ForegroundColor Red
    }
}

if (-not $Simulation) {
    if ((Get-FileHash "$sd\atmosphere\contents\0100000000001000\exefs.nsp" -ErrorAction SilentlyContinue).Hash -ne $homeAvant) { throw 'ALERTE : le menu HOME a changé pendant la copie !' }
    New-Item -ItemType Directory -Force "$sd\config\homenxplus" | Out-Null
    ($etat | ConvertTo-Json) | Set-Content -Encoding UTF8 $etatFichier
    if ($total) { Add-Content -Encoding UTF8 "$sd\VERSIONS.txt" ((Get-Date -Format 'yyyy-MM-dd HH:mm') + " : boutique HomeNX+ : $($Ids -join ', ') ($total fichier(s))") }
    Write-VolumeCache $Lecteur.TrimEnd(':') -ErrorAction SilentlyContinue
}
Remove-Item -Recurse -Force $tmp -ErrorAction SilentlyContinue
Write-Host ''
Write-Host '== Bilan ==' -ForegroundColor Cyan
$bilan | ForEach-Object { Write-Host $_ }
Write-Host "Carte toujours montée sur $sd (non éjectée)."
