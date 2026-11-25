# outils/env.ps1 : prepare l'environnement de compilation HomeNX+ sous Windows.
# A charger avec un point :  . .\outils\env.ps1
# Ne modifie rien de facon permanente : seulement la session PowerShell en cours.

$dkp = 'C:/devkitPro/msys64/opt/devkitpro'
if (-not $env:DEVKITPRO -and (Test-Path $dkp)) { $env:DEVKITPRO = $dkp }

# eSpeak NG (donnees generees sur le PC) a besoin d'un compilateur Windows : MinGW de MSYS2
if (-not $env:CMAKE_GENERATOR) { $env:CMAKE_GENERATOR = 'MinGW Makefiles' }

$ajouts = @(
    "$env:USERPROFILE\xmake",                       # xmake portable
    'C:\devkitPro\msys64\opt\devkitpro\tools\bin',  # elf2nro, nacptool, nxlink...
    'C:\devkitPro\msys64\ucrt64\bin'                # gcc / g++ / mingw32-make pour l'hote
) | Where-Object { (Test-Path $_) -and ($env:PATH -notlike "*$_*") }
if ($ajouts) { $env:PATH = ($ajouts -join ';') + ';' + $env:PATH }
# outils Unix de MSYS2 (sh, tar, make) en dernier, pour ne pas masquer ceux de Windows
if ($env:PATH -notlike '*C:\devkitPro\msys64\usr\bin*') { $env:PATH += ';C:\devkitPro\msys64\usr\bin' }

function Test-HomeNXPlusEnv {
    $ok = $true
    foreach ($c in 'xmake', 'g++', 'git') { if (-not (Get-Command $c -ErrorAction SilentlyContinue)) { Write-Warning "$c introuvable"; $ok = $false } }
    if (-not (Test-Path "$env:DEVKITPRO/devkitA64")) { Write-Warning "devkitA64 introuvable ($env:DEVKITPRO)"; $ok = $false }
    return $ok
}
