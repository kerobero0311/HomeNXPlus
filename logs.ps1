# logs.ps1 : rapatrie les journaux de la Switch par FTP (lecture seule sur la console).
# Usage :  .\logs.ps1 -Ip 192.168.1.42        -> rapports\logs_console\<date_heure>\
# Recupere : homenxplus/logs/, config/SwitchU/*.log, atmosphere/crash_reports/, atmosphere/fatal_reports/
# Ne touche jamais a backup/, aux cles ni a config/homenxplus/api_key.txt.
[CmdletBinding()]
param([Parameter(Mandatory)][string]$Ip, [int]$Port = 5000, [int]$Jours = 7)
$ErrorActionPreference = 'Stop'
$Racine = $PSScriptRoot
. "$Racine\outils\ftp.ps1"
$Base = "ftp://${Ip}:$Port"
$Dest = Join-Path $Racine ("rapports\logs_console\" + (Get-Date -Format 'yyyy-MM-dd_HHmm'))

$sources = @(
    @{ Dossier = 'homenxplus/logs';          Filtre = '*' },
    @{ Dossier = 'config/SwitchU';           Filtre = '*.log' },
    @{ Dossier = 'atmosphere/crash_reports'; Filtre = '*' },
    @{ Dossier = 'atmosphere/fatal_reports'; Filtre = '*' }
)
$n = 0
foreach ($s in $sources) {
    foreach ($f in Get-HnpFtpListe $Base $s.Dossier) {
        if ($f -notlike $s.Filtre -or $f -match 'api_key|prod\.keys|title\.keys|PRODINFO') { continue }
        # les rapports de crash sont horodates (secondes depuis 2000) : on ne garde que les recents
        if ($s.Dossier -like 'atmosphere/*' -and $f -match '^(\d{11})_') {
            $quand = (Get-Date '2000-01-01').AddSeconds([double]$Matches[1])
            if ($quand -lt (Get-Date).AddDays(-$Jours)) { continue }
        }
        Receive-HnpFtpFichier $Base "$($s.Dossier)/$f" (Join-Path $Dest ($s.Dossier -replace '/', '_') | Join-Path -ChildPath $f)
        $n++
    }
}
Write-Host "$n fichier(s) recupere(s) dans $Dest" -ForegroundColor Green
Write-Host "Pense a desactiver sys-ftpd (Ultrahand > Sysmodules)." -ForegroundColor Yellow
