# test.ps1 : tests automatiques sur le PC (sans console).
# Compile chaque test C++ avec le g++ de MSYS2 (hote Windows) et l'execute.
# Usage :  .\test.ps1            (tous les tests)
#          .\test.ps1 -Filtre sse (seulement les tests dont le nom contient "sse")
[CmdletBinding()]
param([string]$Filtre = '')

$ErrorActionPreference = 'Continue'   # g++ ecrit sur stderr : ne pas en faire une exception
$Racine = $PSScriptRoot
. "$Racine\outils\env.ps1"
if (-not (Test-HomeNXPlusEnv)) { Write-Host "Environnement incomplet (voir avertissements)." -ForegroundColor Red; exit 2 }

$tmp = Join-Path $Racine 'build\tests'
New-Item -ItemType Directory -Force $tmp | Out-Null

# nlohmann/json : portlibs devkitPro ou paquet xmake
$json = @("$env:DEVKITPRO/portlibs/switch/include") + @(Get-ChildItem "$env:LOCALAPPDATA\.xmake\packages\n\nlohmann_json" -Recurse -Directory -Filter include -ErrorAction SilentlyContinue | ForEach-Object FullName) |
    Where-Object { Test-Path "$_/nlohmann/json.hpp" } | Select-Object -First 1
$nx = "$env:DEVKITPRO/libnx/include"

# nom -> sources et include supplementaires
$tests = [ordered]@{
    # tests du projet d'origine (HomeNX)
    'grid_navigation'        = @{ src = @('tests/grid_navigation_test.cpp', 'projects/menu/src/widgets/GridNavigation.cpp'); inc = @('projects/menu/src', 'projects/common/include') }
    'widget_model'           = @{ src = @('tests/widget_model_test.cpp', 'projects/menu/src/core/WidgetModel.cpp'); inc = @('projects/menu/src', 'projects/common/include', $json) }
    'cards_personalization'  = @{ windows = 'dossier /tmp POSIX (mkdtemp) : test lance par la CI Linux du projet'; src = @('tests/cards_personalization_test.cpp'); inc = @('projects/cards/src', 'projects/common/include', $json) }
    'lock_screen_wake'       = @{ windows = 'dossier /tmp POSIX (mkdtemp) : test lance par la CI Linux du projet'; src = @('tests/lock_screen_wake_test.cpp'); inc = @('projects/common/include', $json) }
    'software_move'          = @{ src = @('tests/software_move_test.cpp'); inc = @('tests/support', 'projects/common/include', $nx) }
}
# tests HomeNX+ : chaque tests/homenxplus/*_test.cpp, avec les sources de projects/homenxplus/src
Get-ChildItem "$Racine\tests\homenxplus" -Filter '*_test.cpp' -ErrorAction SilentlyContinue | ForEach-Object {
    $libs = @(Get-ChildItem "$Racine\projects\homenxplus\src" -Recurse -Filter '*.cpp' -ErrorAction SilentlyContinue |
        Where-Object { $_.FullName -notmatch '\\switch\\' } | ForEach-Object { $_.FullName.Substring($Racine.Length + 1) })
    $tests["hnp_" + ($_.BaseName -replace '_test$', '')] = @{ src = @($_.FullName.Substring($Racine.Length + 1)) + $libs; inc = @('projects/homenxplus/include', 'tests/support', $json) }
}

$ok = 0; $ko = @()
Push-Location $Racine
try {
    foreach ($nom in $tests.Keys) {
        if ($Filtre -and $nom -notlike "*$Filtre*") { continue }
        $t = $tests[$nom]
        if ($t.windows) { Write-Host "[--] $nom : saute sous Windows ($($t.windows))" -ForegroundColor Yellow; continue }
        if ($t.inc -contains $null) { Write-Host "[--] $nom : nlohmann/json introuvable, test saute" -ForegroundColor Yellow; continue }
        $exe = Join-Path $tmp "$nom.exe"
        # \" dans les -D : PowerShell 5.1 retire sinon les guillemets passes a g++
        $args = @('-std=gnu++20', '-O0', '-g', '-Wall', '-DHNP_CFG_SWITCHU=\"sdmc:/config/homenxplus/menu\"', '-DHNP_SW_DATA=\"sdmc:/switch/HomeNXPlus/data\"') + ($t.inc | ForEach-Object { "-I$_" }) + $t.src + @('-o', $exe)
        $sortie = & g++ @args 2>&1
        if ($LASTEXITCODE -ne 0) { $ko += "$nom (compilation)"; Write-Host "[KO] $nom : compilation" -ForegroundColor Red; $sortie | Select-Object -First 15 | ForEach-Object { "     $_" }; continue }
        $run = & $exe 2>&1
        if ($LASTEXITCODE -ne 0) { $ko += "$nom (execution)"; Write-Host "[KO] $nom : echec (code $LASTEXITCODE)" -ForegroundColor Red; $run | Select-Object -First 15 | ForEach-Object { "     $_" } }
        else { $ok++; Write-Host "[OK] $nom" -ForegroundColor Green }
    }
    # pc_agent (Python, bibliotheque standard)
    if (-not $Filtre -or 'pc_agent' -like "*$Filtre*") {
        $py = Get-Command py -ErrorAction SilentlyContinue
        if (-not $py) { Write-Host "[--] pc_agent : Python (py) introuvable, test saute" -ForegroundColor Yellow }
        else {
            $sortie = & py -I "$Racine\pc_agent\test_agent.py" 2>&1 | Out-String
            if ($LASTEXITCODE -eq 0) { $ok++; Write-Host "[OK] pc_agent" -ForegroundColor Green }
            else { $ko += 'pc_agent'; Write-Host "[KO] pc_agent" -ForegroundColor Red; ($sortie -split "`n") | Select-Object -Last 15 | ForEach-Object { "     $_" } }
        }
    }
} finally { Pop-Location }

Write-Host ""
Write-Host ("{0} test(s) OK, {1} en echec" -f $ok, $ko.Count)
if ($ko) { $ko | ForEach-Object { Write-Host "  - $_" }; exit 1 }
exit 0
