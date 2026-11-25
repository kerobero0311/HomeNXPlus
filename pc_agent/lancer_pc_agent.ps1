# lancer_pc_agent.ps1 : demarre le compagnon PC de HomeNX+ (fenetre a laisser ouverte).
# Au premier lancement, Windows peut demander d'autoriser Python sur le reseau PRIVE : accepte
# (reseau prive uniquement), sinon la Switch ne pourra pas joindre le PC.
$ErrorActionPreference = 'Stop'
$py = Get-Command py -ErrorAction SilentlyContinue
if (-not $py) { Write-Host "Python (py) introuvable." -ForegroundColor Red; exit 1 }
& py -I (Join-Path $PSScriptRoot 'agent.py')
