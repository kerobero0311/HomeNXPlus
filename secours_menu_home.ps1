# secours_menu_home.ps1 : remet le menu HOME Nintendo d'origine si le menu perso (HomeNX / HomeNX+) bloque.
# Carte SD dans le lecteur du PC (ou mode UMS via Ultrahand / Hekate).
# Usage :  .\secours_menu_home.ps1                  -> desactive le menu perso
#          .\secours_menu_home.ps1 -Action reactiver
# Methode : renommer exefs.nsp en exefs.nsp.disabled (comme NXToggle). Rien n'est supprime.
[CmdletBinding()]
param([ValidateSet('desactiver', 'reactiver')][string]$Action = 'desactiver', [string]$Lecteur = '')

if (-not $Lecteur) {
    $v = Get-Volume | Where-Object { $_.FileSystemLabel -eq 'SWITCH SD' -and $_.DriveLetter } | Select-Object -First 1
    if (-not $v) { Write-Host "Carte 'SWITCH SD' introuvable. Branche-la ou precise -Lecteur Z" -ForegroundColor Red; exit 1 }
    $Lecteur = $v.DriveLetter
}
$dir = "${Lecteur}:\atmosphere\contents\0100000000001000"
$nsp = Join-Path $dir 'exefs.nsp'; $off = Join-Path $dir 'exefs.nsp.disabled'
function Add-Note($m) { Add-Content -LiteralPath "${Lecteur}:\VERSIONS.txt" -Encoding UTF8 -Value "$(Get-Date -Format 'dd/MM/yyyy HH:mm') - $m" }

if ($Action -eq 'desactiver') {
    if (Test-Path $nsp) {
        Rename-Item $nsp 'exefs.nsp.disabled'
        Add-Note "Secours (PC) : menu HOME perso desactive (exefs.nsp -> exefs.nsp.disabled)."
        Write-Host "OK : au prochain demarrage, la console utilisera le menu HOME Nintendo." -ForegroundColor Green
    } elseif (Test-Path $off) { Write-Host "Deja desactive : rien a faire." }
    else { Write-Host "Aucun menu HOME perso trouve : rien a faire." }
} else {
    if ((Test-Path $off) -and -not (Test-Path $nsp)) {
        Rename-Item $off 'exefs.nsp'
        Add-Note "Secours (PC) : menu HOME perso reactive (exefs.nsp.disabled -> exefs.nsp)."
        Write-Host "OK : le menu HOME perso reviendra au prochain demarrage." -ForegroundColor Green
    } else { Write-Host "Rien a reactiver." }
}
Write-VolumeCache -DriveLetter $Lecteur -ErrorAction SilentlyContinue
