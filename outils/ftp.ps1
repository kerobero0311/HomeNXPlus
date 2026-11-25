# outils/ftp.ps1 : petites fonctions FTP (sys-ftpd / ftpd sur la Switch).
# A charger avec un point. Identifiants : parametres, ou variables HNP_FTP_USER / HNP_FTP_PASS
# (jamais ecrits dans un journal). Par defaut : connexion anonyme.

function New-HnpFtpRequest([string]$Url, [string]$Methode) {
    $r = [System.Net.FtpWebRequest]::Create($Url)
    $r.Method = $Methode
    $r.UseBinary = $true
    $r.UsePassive = $true
    $r.KeepAlive = $false
    $r.Timeout = 15000
    $u = if ($env:HNP_FTP_USER) { $env:HNP_FTP_USER } else { 'anonymous' }
    $p = if ($env:HNP_FTP_PASS) { $env:HNP_FTP_PASS } else { 'anonymous' }
    $r.Credentials = New-Object System.Net.NetworkCredential($u, $p)
    return $r
}

function Invoke-HnpFtp([string]$Url, [string]$Methode) {
    $resp = (New-HnpFtpRequest $Url $Methode).GetResponse()
    $flux = New-Object IO.StreamReader($resp.GetResponseStream())
    $texte = $flux.ReadToEnd(); $flux.Close(); $resp.Close()
    return $texte
}

function Get-HnpFtpListe([string]$Base, [string]$Dossier) {
    try { (Invoke-HnpFtp "$Base/$Dossier/" ([System.Net.WebRequestMethods+Ftp]::ListDirectory)) -split "`r?`n" | Where-Object { $_ -and $_ -notin '.', '..' } | ForEach-Object { Split-Path $_ -Leaf } }
    catch { @() }
}

function Receive-HnpFtpFichier([string]$Base, [string]$Distant, [string]$Local) {
    $resp = (New-HnpFtpRequest "$Base/$Distant" ([System.Net.WebRequestMethods+Ftp]::DownloadFile)).GetResponse()
    New-Item -ItemType Directory -Force (Split-Path $Local) | Out-Null
    $out = [IO.File]::Create($Local); $resp.GetResponseStream().CopyTo($out); $out.Close(); $resp.Close()
}

function Send-HnpFtpFichier([string]$Base, [string]$Local, [string]$Distant) {
    $req = New-HnpFtpRequest "$Base/$Distant" ([System.Net.WebRequestMethods+Ftp]::UploadFile)
    $data = [IO.File]::ReadAllBytes($Local); $req.ContentLength = $data.Length
    $s = $req.GetRequestStream(); $s.Write($data, 0, $data.Length); $s.Close(); $req.GetResponse().Close()
}

function Add-HnpFtpTexte([string]$Base, [string]$Distant, [string]$Texte) {
    $req = New-HnpFtpRequest "$Base/$Distant" ([System.Net.WebRequestMethods+Ftp]::AppendFile)
    $data = [Text.Encoding]::UTF8.GetBytes($Texte); $req.ContentLength = $data.Length
    $s = $req.GetRequestStream(); $s.Write($data, 0, $data.Length); $s.Close(); $req.GetResponse().Close()
}

function Rename-HnpFtp([string]$Base, [string]$Distant, [string]$NouveauNom) {
    $req = New-HnpFtpRequest "$Base/$Distant" ([System.Net.WebRequestMethods+Ftp]::Rename)
    $req.RenameTo = $NouveauNom; $req.GetResponse().Close()
}

function Remove-HnpFtpFichier([string]$Base, [string]$Distant) {
    try { (New-HnpFtpRequest "$Base/$Distant" ([System.Net.WebRequestMethods+Ftp]::DeleteFile)).GetResponse().Close() } catch { }
}

function New-HnpFtpDossier([string]$Base, [string]$Distant) {
    try { (New-HnpFtpRequest "$Base/$Distant" ([System.Net.WebRequestMethods+Ftp]::MakeDirectory)).GetResponse().Close() } catch { }  # existe deja
}
