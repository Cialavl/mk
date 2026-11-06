#requires -Version 5.1
<#
.SYNOPSIS
    下载 Index-Translate GGUF 模型（hf-mirror，断点续传，SHA256 校验）。
.DESCRIPTION
    默认下载 Index-Translate-9B Q4_K_M（5.78GB）。单连接 + 退避重试，
    避免 hf-mirror 对多连接的 429 限流。已存在的部分文件自动续传。
.EXAMPLE
    pwsh -File scripts/fetch-model.ps1
    pwsh -File scripts/fetch-model.ps1 -File Index-Translate-2B.Q4_K_M.gguf -Dest resources/model/index-2b -ExpectedSize 1312164352 -Sha256 044b313d29342bd3b2c77cbb64023ca0d209bd9b3247763f9d162767ef2d746a
#>
[CmdletBinding()]
param(
    [string]$Repo = "IndexTeam/Index-Translate-9B-GGUF",
    [string]$File = "Index-Translate-9B.Q4_K_M.gguf",
    [string]$Dest = "resources/model/index-9b",
    [string]$Endpoint = "https://hf-mirror.com",
    [long]$ExpectedSize = 5780090304,
    [string]$Sha256 = "9cc6758b0007f4cbf42ea3400768cbff91ae74ef8d640e467158348426d33713"
)

$ErrorActionPreference = "Stop"
$out = Join-Path $Dest $File
$url = "$Endpoint/$Repo/resolve/main/$($File)?download=true"
New-Item -ItemType Directory -Force -Path $Dest | Out-Null

Write-Host "[fetch] $url"
Write-Host "[fetch] -> $out ($([math]::Round($ExpectedSize / 1GB, 2)) GB)"

for ($attempt = 1; $attempt -le 80; $attempt++) {
    if ((Test-Path $out) -and ((Get-Item $out).Length -eq $ExpectedSize)) { break }

    $have = 0
    if (Test-Path $out) { $have = (Get-Item $out).Length }
    Write-Host "[fetch] attempt $attempt, have $([math]::Round($have / 1MB, 1)) MB"

    # -f: HTTP 错误返回非 0；-C -: 续传；--retry: 429/5xx 退避重试
    & curl.exe -L -f -C - -s -S --connect-timeout 30 --max-time 900 --speed-limit 20480 --speed-time 45 --retry 3 --retry-delay 15 --retry-all-errors -o $out $url

    if ($LASTEXITCODE -eq 0) { break }
    if ($LASTEXITCODE -eq 33) { Remove-Item $out -Force -ErrorAction SilentlyContinue }
    Start-Sleep -Seconds 45
}

if (-not (Test-Path $out)) { throw "download failed: $out missing" }
$len = (Get-Item $out).Length
if ($len -ne $ExpectedSize) { throw "size mismatch: $len != $ExpectedSize" }

$hash = (Get-FileHash -Algorithm SHA256 $out).Hash.ToLowerInvariant()
if ($Sha256 -and $hash -ne $Sha256.ToLowerInvariant()) { throw "sha256 mismatch: $hash" }

Write-Host "[fetch] OK $out"
