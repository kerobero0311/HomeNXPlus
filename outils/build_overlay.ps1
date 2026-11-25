# outils/build_overlay.ps1 : compile l'overlay HomeNX+ (projects/overlay -> HomeNXPlus.ovl) avec make (MSYS2).
$ErrorActionPreference = 'Stop'
$Racine = Split-Path $PSScriptRoot -Parent
$bash = 'C:\devkitPro\msys64\usr\bin\bash.exe'
$proj = & 'C:\devkitPro\msys64\usr\bin\cygpath.exe' -u (Join-Path $Racine 'projects\overlay')
& $bash -lc "export DEVKITPRO=/opt/devkitpro; export PATH=/opt/devkitpro/tools/bin:/opt/devkitpro/devkitA64/bin:`$PATH; cd '$proj' && make -j4"
if ($LASTEXITCODE -ne 0) { throw "Compilation de l'overlay en echec" }
Get-Item (Join-Path $Racine 'projects\overlay\HomeNXPlus.ovl') | ForEach-Object { "{0} ({1:N0} Ko)" -f $_.FullName, ($_.Length / 1KB) }
