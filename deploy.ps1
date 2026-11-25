# deploy.ps1 : envoie HomeNX+ (.nro et .ovl) sur la Switch par FTP, sans retirer la carte.
# Avant : activer sys-ftpd (Ultrahand > Sysmodules) ; apres : le desactiver.
# Usage :  .\deploy.ps1 -Ip 192.168.1.42            (port 5000 par defaut)
#          .\deploy.ps1 -Ip 192.168.1.42 -Simulation (affiche ce qui serait envoye)
# Regles (CLAUDE.md) : n'ecrit QUE dans switch/HomeNXPlus/ et switch/.overlays/HomeNXPlus.ovl,
# envoi dans un fichier .tmp puis renommage, une ligne ajoutee a VERSIONS.txt.
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$Ip,
    [int]$Port = 5000,
    [ValidateSet('release', 'debug')][string]$Mode = 'release',
    [switch]$Simulation
)
$ErrorActionPreference = 'Stop'
$Racine = $PSScriptRoot
. "$Racine\outils\ftp.ps1"
$Base = "ftp://${Ip}:$Port"

# Fichiers a envoyer : local -> distant (liste fermee)
$plan = @()
$nro = Get-ChildItem "$Racine\build\homebrew" -Recurse -Filter 'HomeNX*.nro' -ErrorAction SilentlyContinue | Where-Object { $_.Name -in 'HomeNXPlus.nro', 'HomeNX+.nro' -and $_.FullName -match "\\$Mode\\" } | Select-Object -First 1
if (-not $nro) { $nro = Get-ChildItem "$Racine\build\homebrew" -Recurse -Filter 'HomeNX.nro' -ErrorAction SilentlyContinue | Where-Object { $_.FullName -match "\\$Mode\\" } | Select-Object -First 1 }
if ($nro) { $plan += [pscustomobject]@{ Local = $nro.FullName; Distant = 'switch/HomeNXPlus/HomeNXPlus.nro' } }
$ovl = Get-Item "$Racine\projects\overlay\HomeNXPlus.ovl" -ErrorAction SilentlyContinue
if ($ovl) { $plan += [pscustomobject]@{ Local = $ovl.FullName; Distant = 'switch/.overlays/HomeNXPlus.ovl' } }
if (-not $plan) { Write-Host "Rien a envoyer : lance d'abord .\build.ps1 -Variant homebrew" -ForegroundColor Red; exit 1 }

# Garde-fou : destinations autorisees uniquement
$autorise = '^(switch/HomeNXPlus/[^/]+|switch/\.overlays/HomeNXPlus\.ovl)$'
foreach ($p in $plan) { if ($p.Distant -notmatch $autorise) { throw "Destination interdite : $($p.Distant)" } }

foreach ($p in $plan) { "{0,-45} -> {1} ({2:N1} Mo)" -f (Split-Path $p.Local -Leaf), $p.Distant, ((Get-Item $p.Local).Length / 1MB) }
if ($Simulation) { Write-Host "Simulation : rien n'a ete envoye." -ForegroundColor Yellow; exit 0 }

try { [void](Get-HnpFtpListe $Base 'switch') } catch { }
New-HnpFtpDossier $Base 'switch/HomeNXPlus'
foreach ($p in $plan) {
    $tmp = "$($p.Distant).tmp"
    Send-HnpFtpFichier $Base $p.Local $tmp
    Remove-HnpFtpFichier $Base $p.Distant
    Rename-HnpFtp $Base $tmp (Split-Path $p.Distant -Leaf)
    Write-Host "envoye : $($p.Distant)" -ForegroundColor Green
}
$h = (Get-FileHash $plan[0].Local -Algorithm SHA256).Hash.Substring(0, 12)
$ligne = "`r`n$(Get-Date -Format 'dd/MM/yyyy HH:mm') - HomeNX+ (deploy.ps1, FTP) : $(($plan | ForEach-Object Distant) -join ', ') [sha256 $h]"
Add-HnpFtpTexte $Base 'VERSIONS.txt' $ligne
Write-Host "VERSIONS.txt mis a jour. Pense a desactiver sys-ftpd (Ultrahand > Sysmodules)." -ForegroundColor Yellow
