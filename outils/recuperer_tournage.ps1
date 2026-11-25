# HomeNX+ : récupère les captures du tournage (photos et vidéos de l'Album, vidéos filmées sous Linux)
# depuis la carte SD (lecteur Z: de l'Ally) vers promo\tournage\rushes, rangées par heure de prise.
# Lecture seule sur la carte : rien n'y est modifié ni supprimé.
#   .\outils\recuperer_tournage.ps1                 # tout ce qui a été pris aujourd'hui
#   .\outils\recuperer_tournage.ps1 -Depuis 2026-11-25
param(
    [string]$Carte = 'Z:',
    [datetime]$Depuis = (Get-Date).Date
)
$ErrorActionPreference = 'Stop'
$Racine = Split-Path -Parent $PSScriptRoot
$Dest = Join-Path $Racine 'promo\tournage\rushes'
New-Item -ItemType Directory -Force $Dest | Out-Null

$videosPc = Join-Path ([Environment]::GetFolderPath('MyVideos')) 'Switch Frame Tap'   # enregistrements du PC
$sources = @("$Carte\emuMMC\RAW1\Nintendo\Album", "$Carte\Nintendo\Album", "$Carte\homenxplus\tournage", $videosPc, (Join-Path $Racine 'promo\tournage\telephone')) | Where-Object { Test-Path $_ }
if (-not $sources) { Write-Host "Aucune source : ni carte SD sur $Carte, ni vidéos Switch Frame Tap, ni vidéos du téléphone." -ForegroundColor Red; exit 1 }

$fichiers = foreach ($s in $sources) {
    Get-ChildItem $s -Recurse -File -Include *.jpg, *.png, *.mp4, *.mov | Where-Object { $_.LastWriteTime -ge $Depuis }
}
$fichiers = $fichiers | Sort-Object LastWriteTime
if (-not $fichiers) { Write-Host "Aucune capture depuis le $($Depuis.ToString('dd/MM/yyyy'))." ; exit 0 }

$n = 0; $copies = 0
foreach ($f in $fichiers) {
    $n++
    $nom = '{0:D3}_{1}_{2}' -f $n, $f.LastWriteTime.ToString('HHmmss'), $f.Name
    $cible = Join-Path $Dest $nom
    if (Get-ChildItem $Dest -Filter "*_$($f.Name)" -ErrorAction SilentlyContinue) { continue }  # déjà récupérée
    Copy-Item $f.FullName $cible
    $copies++
}
$v = ($fichiers | Where-Object Extension -eq '.mp4').Count
Write-Host "$copies nouvelle(s) capture(s) copiée(s) ($v vidéo(s) au total) dans $Dest" -ForegroundColor Green
