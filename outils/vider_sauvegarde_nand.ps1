# vider_sauvegarde_nand.ps1 : deplace les morceaux de sauvegarde NAND de Hekate (carte SD -> PC),
# en verifiant chaque copie (SHA-256) avant de supprimer le morceau sur la carte.
# Ne lit ni n'affiche le contenu des fichiers ; ne touche qu'a backup/<id>/ (rawnand.bin*, BOOT0, BOOT1).
[CmdletBinding()]
param([string]$Lecteur = '', [string]$Destination = "$env:USERPROFILE\Documents\Sauvegarde_NAND_Switch")
$ErrorActionPreference = 'Stop'
if (-not $Lecteur) {
    $v = Get-Volume | Where-Object { $_.FileSystemLabel -eq 'SWITCH SD' -and $_.DriveLetter } | Select-Object -First 1
    if (-not $v) { Write-Host "Carte 'SWITCH SD' introuvable." -ForegroundColor Red; exit 1 }
    $Lecteur = $v.DriveLetter
}
# Hekate ecrit dans backup/<identifiant>/ (partial : backup/<id>/partial/)
$fichiers = Get-ChildItem "${Lecteur}:\backup" -Recurse -File -ErrorAction SilentlyContinue |
    Where-Object { $_.Name -match '^(rawnand\.bin(\.\d+)?|BOOT0|BOOT1)$' }
if (-not $fichiers) { Write-Host "Aucun morceau de sauvegarde NAND sur la carte." -ForegroundColor Yellow; exit 0 }

$jour = Get-Date -Format 'yyyy-MM-dd'
$dest = Join-Path $Destination $jour
New-Item -ItemType Directory -Force $dest | Out-Null
$libre = (Get-PSDrive ($dest.Substring(0, 1))).Free
$total = ($fichiers | Measure-Object Length -Sum).Sum
if ($libre -lt $total + 1GB) { Write-Host ("Pas assez de place sur le PC : {0:N1} Go necessaires." -f ($total / 1GB)) -ForegroundColor Red; exit 1 }

foreach ($f in $fichiers) {
    $cible = Join-Path $dest $f.Name
    if (Test-Path $cible) {
        if ((Get-FileHash $cible).Hash -eq (Get-FileHash $f.FullName).Hash) { Remove-Item $f.FullName; Write-Host "deja copie, libere : $($f.Name)"; continue }
        $cible = Join-Path $dest ("{0}.{1}" -f $f.Name, (Get-Date -Format HHmmss))
    }
    Copy-Item $f.FullName $cible
    if ((Get-FileHash $cible).Hash -ne (Get-FileHash $f.FullName).Hash) { Write-Host "ERREUR de copie : $($f.Name) (laisse sur la carte)" -ForegroundColor Red; continue }
    # BOOT0/BOOT1 sont petits : on les garde aussi sur la carte (utile a Hekate)
    if ($f.Name -match '^rawnand') { Remove-Item $f.FullName }
    Write-Host ("OK {0} ({1:N2} Go)" -f $f.Name, ($f.Length / 1GB)) -ForegroundColor Green
}
Add-Content -LiteralPath "${Lecteur}:\VERSIONS.txt" -Encoding UTF8 -Value "$(Get-Date -Format 'dd/MM/yyyy HH:mm') - Sauvegarde NAND : $($fichiers.Count) morceau(x) deplaces vers le PC (verifies SHA-256)."
Write-VolumeCache -DriveLetter $Lecteur -ErrorAction SilentlyContinue
Write-Host "Termine. Remets la carte dans la Switch et relance eMMC RAW GPP dans Hekate s'il n'a pas fini." -ForegroundColor Cyan
