# outils/installer_sur_sd.ps1 : copie les livrables HomeNX+ sur la carte SD (lecteur du PC ou mode UMS).
# Ecrit UNIQUEMENT dans les dossiers autorises par CLAUDE.md (regle 4), fichier .tmp puis renommage,
# verification SHA-256, une ligne dans VERSIONS.txt. Ne touche jamais au menu HOME reel.
[CmdletBinding()]
param([string]$Lecteur = '', [switch]$Simulation)
$ErrorActionPreference = 'Stop'
$Racine = Split-Path $PSScriptRoot -Parent
if (-not $Lecteur) {
    $v = Get-Volume | Where-Object { $_.FileSystemLabel -eq 'SWITCH SD' -and $_.DriveLetter } | Select-Object -First 1
    if (-not $v) { Write-Host "Carte 'SWITCH SD' introuvable." -ForegroundColor Red; exit 1 }
    $Lecteur = $v.DriveLetter
}
$sd = "${Lecteur}:"
$rel = Join-Path $Racine 'build\homebrew\cross\aarch64\release'

# local -> destination sur la carte ; lf = convertir en fins de ligne Linux ; garder = ne pas ecraser
$plan = @(
    @{ src = "$rel\HomeNX+.nro";                         dst = 'switch\HomeNXPlus\HomeNXPlus.nro' },
    @{ src = "$rel\ClaudeNX.nro";                        dst = 'switch\ClaudeNX\ClaudeNX.nro' },
    @{ src = "$Racine\assets\cacert.pem";                dst = 'config\homenxplus\cacert.pem' },
    @{ src = "$Racine\projects\overlay\HomeNXPlus.ovl";  dst = 'switch\.overlays\HomeNXPlus.ovl' },
    @{ src = "$Racine\linux\secours_menu_home.sh";       dst = 'switchroot\outils\homenxplus\secours_menu_home.sh'; lf = $true },
    @{ src = "$Racine\linux\rapport_sante.sh";           dst = 'switchroot\outils\homenxplus\rapport_sante.sh'; lf = $true },
    @{ src = "$Racine\linux\boite_aux_lettres.sh";       dst = 'switchroot\outils\homenxplus\boite_aux_lettres.sh'; lf = $true },
    @{ src = "$Racine\linux\installer.sh";               dst = 'switchroot\outils\homenxplus\installer.sh'; lf = $true },
    @{ src = "$Racine\linux\filmer_ecran.sh";           dst = 'switchroot\outils\homenxplus\filmer_ecran.sh'; lf = $true }
)
$autorise = '^(switch\\HomeNXPlus\\|switch\\ClaudeNX\\|switch\\\.overlays\\HomeNXPlus\.ovl$|homenxplus\\|config\\homenxplus\\|switchroot\\outils\\homenxplus\\)'
foreach ($p in $plan) {
    if ($p.dst -notmatch $autorise) { throw "Destination interdite : $($p.dst)" }
    if (-not (Test-Path $p.src)) { throw "Fichier manquant (compiler d'abord) : $($p.src)" }
}
$exefsAvant = (Get-FileHash "$sd\atmosphere\contents\0100000000001000\exefs.nsp" -ErrorAction SilentlyContinue).Hash
$total = ($plan | ForEach-Object { (Get-Item $_.src).Length } | Measure-Object -Sum).Sum
$libre = (Get-Volume -DriveLetter $Lecteur).SizeRemaining
"{0} fichier(s), {1:N1} Mo ; libre sur la carte : {2:N2} Go" -f $plan.Count, ($total / 1MB), ($libre / 1GB)
if ($libre -lt $total + 500MB) { throw "Pas assez de place sur la carte." }
if ($Simulation) { $plan | ForEach-Object { "  $($_.src) -> $sd\$($_.dst)" }; exit 0 }

$lignes = @()
foreach ($p in $plan) {
    $dst = Join-Path $sd $p.dst; $tmp = "$dst.tmp"
    New-Item -ItemType Directory -Force (Split-Path $dst) | Out-Null
    if ($p.lf) { [IO.File]::WriteAllText($tmp, ([IO.File]::ReadAllText($p.src) -replace "`r`n", "`n"), (New-Object Text.UTF8Encoding $false)) }
    else { Copy-Item $p.src $tmp -Force }
    Move-Item $tmp $dst -Force
    $ok = $p.lf -or ((Get-FileHash $p.src).Hash -eq (Get-FileHash $dst).Hash)
    if (-not $ok) { throw "Copie differente : $($p.dst)" }
    $h = (Get-FileHash $dst).Hash.Substring(0, 12)
    "  OK $($p.dst) [$h]"; $lignes += "$($p.dst.Replace('\','/')) [$h]"
}
# Lien avec le PC : pc_agent.ini (IP, port, jeton, MAC) genere par pc_agent\config_switch.py
if (Get-Command py -ErrorAction SilentlyContinue) {
    $ini = & py -I (Join-Path $Racine 'pc_agent\config_switch.py') 2>$null | Out-String
    if ($ini -match 'jeton = \S{32,}') {
        New-Item -ItemType Directory -Force "$sd\config\homenxplus" | Out-Null
        [IO.File]::WriteAllText("$sd\config\homenxplus\pc_agent.ini.tmp", $ini, (New-Object Text.UTF8Encoding $false))
        Move-Item "$sd\config\homenxplus\pc_agent.ini.tmp" "$sd\config\homenxplus\pc_agent.ini" -Force
        "  OK config\homenxplus\pc_agent.ini (lien PC ; jeton non affiche)"; $lignes += 'config/homenxplus/pc_agent.ini'
    }
}
foreach ($d in 'homenxplus\logs', 'homenxplus\chats', 'homenxplus\claude', 'homenxplus\health', 'homenxplus\mailbox\inbox', 'homenxplus\mailbox\outbox') { New-Item -ItemType Directory -Force (Join-Path $sd $d) | Out-Null }
if ((Get-FileHash "$sd\atmosphere\contents\0100000000001000\exefs.nsp" -ErrorAction SilentlyContinue).Hash -ne $exefsAvant) { throw "ALERTE : le menu HOME a change pendant la copie !" }
Add-Content -LiteralPath "$sd\VERSIONS.txt" -Encoding UTF8 -Value "$(Get-Date -Format 'dd/MM/yyyy HH:mm') - HomeNX+ (installer_sur_sd.ps1) : $($lignes -join ', '). Menu HOME reel non modifie."
Write-VolumeCache -DriveLetter $Lecteur
Write-Host "Termine. Menu HOME reel verifie inchange." -ForegroundColor Green
