#requires -Version 5.1
<#
.SYNOPSIS
    Download a prebuilt llama.cpp CUDA build and generate llama.lib.
.DESCRIPTION
    CMake can use third_party/llama-prebuilt/llama.lib as an imported target.
    The release zip ships llama.dll but no import library, so this script
    creates llama.lib with dumpbin + lib from Visual Studio.
.EXAMPLE
    pwsh -File scripts/fetch-llama.ps1
    pwsh -File scripts/fetch-llama.ps1 -Proxy http://127.0.0.1:7897
#>
[CmdletBinding()]
param(
    [string]$Tag = "b12144",
    [string]$Commit = "3ed7284f2d9e22b2d1a7d05c1416c00c5a9f0bc7",
    [string]$Dest = "third_party/llama-prebuilt",
    [string]$Proxy = ""
)

$ErrorActionPreference = "Stop"

$zipName = "llama-$Tag-bin-win-cuda-13.4-x64.zip"
$zip = Join-Path "third_party" $zipName
$url = "https://github.com/ggml-org/llama.cpp/releases/download/$Tag/$zipName"

New-Item -ItemType Directory -Force "third_party" | Out-Null

if (-not (Test-Path $zip)) {
    $curlArgs = @("-L", "-C", "-", "-s", "-S", "--max-time", "900", "--speed-limit", "20480", "--speed-time", "60", "-o", $zip, $url)
    if ($Proxy) { $curlArgs = @("-x", $Proxy) + $curlArgs }
    & curl.exe @curlArgs
    if ($LASTEXITCODE -ne 0) { throw "download failed: $url" }
}

if (-not (Test-Path $Dest)) {
    Expand-Archive -Path $zip -DestinationPath $Dest
}

# Headers are needed to compile against the prebuilt DLL.
$headers = "third_party/llama.cpp/include/llama.h"
if (-not (Test-Path $headers)) {
    $srcZip = "third_party/llama-src.zip"
    $srcUrl = "https://github.com/ggml-org/llama.cpp/archive/$Commit.zip"
    if (-not (Test-Path $srcZip)) {
        $srcArgs = @("-L", "-C", "-", "-s", "-S", "--max-time", "900", "-o", $srcZip, $srcUrl)
        if ($Proxy) { $srcArgs = @("-x", $Proxy) + $srcArgs }
        & curl.exe @srcArgs
        if ($LASTEXITCODE -ne 0) { throw "download failed: $srcUrl" }
    }
    $tmp = "third_party/llama-src-tmp"
    Remove-Item -Recurse -Force $tmp -ErrorAction SilentlyContinue
    Expand-Archive -Path $srcZip -DestinationPath $tmp -Force
    $inner = Get-ChildItem $tmp -Directory | Select-Object -First 1
    Move-Item $inner.FullName "third_party/llama.cpp"
    Remove-Item -Recurse -Force $tmp
}

$abs = (Resolve-Path $Dest).Path
if (-not (Test-Path (Join-Path $abs "llama.dll"))) { throw "llama.dll missing in $abs" }
if (Test-Path (Join-Path $abs "llama.lib")) {
    Write-Host "[llama] already prepared: $abs"
    return
}

$pf86 = [Environment]::GetFolderPath([Environment+SpecialFolder]::ProgramFilesX86)
$vswhere = Join-Path $pf86 "Microsoft Visual Studio/Installer/vswhere.exe"
if (-not (Test-Path $vswhere)) { throw "vswhere.exe not found" }
$vs = & $vswhere -latest -products * -property installationPath
if (-not $vs) { throw "Visual Studio not found" }
$vcvars = Join-Path $vs "VC/Auxiliary/Build/vcvars64.bat"

cmd.exe /d /c ('"' + $vcvars + '" >nul && cd /d "' + $abs + '" && dumpbin /exports llama.dll > exports.txt')
if ($LASTEXITCODE -ne 0) { throw "dumpbin failed" }

$names = Get-Content (Join-Path $abs "exports.txt") | ForEach-Object {
    if ($_ -match '^\s+\d+\s+[0-9A-Fa-f]+\s+[0-9A-Fa-f]+\s+(\S+)') { $matches[1] }
}
if (-not $names) { throw "no exports parsed from llama.dll" }

$def = @("LIBRARY llama", "EXPORTS") + $names
Set-Content -Path (Join-Path $abs "llama.def") -Value $def -Encoding Ascii

cmd.exe /d /c ('"' + $vcvars + '" >nul && cd /d "' + $abs + '" && lib /def:llama.def /machine:x64 /out:llama.lib')
if ($LASTEXITCODE -ne 0) { throw "lib.exe failed" }

Write-Host "[llama] prepared $abs ($($names.Count) exports)"
