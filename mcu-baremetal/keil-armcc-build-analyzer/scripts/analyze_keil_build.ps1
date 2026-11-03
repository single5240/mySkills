# analyze_keil_build.ps1
# Zero-dependency PowerShell script to parse Keil MDK (.build_log.htm) and .map files.
param(
    [Parameter(Mandatory = $true)]
    [string]$LogPath,
    [string]$MapPath = "",
    [string]$SaveBaseline = "",
    [string]$CompareBaseline = "",
    [int]$Top = 8
)

$ErrorActionPreference = "Stop"

if (-not (Test-Path $LogPath)) {
    Write-Error "Build log not found: $LogPath"
    exit 2
}

$raw = Get-Content -Path $LogPath -Raw -Encoding UTF8
$clean = $raw -replace '<[^>]+>', ''
$clean = [System.Net.WebUtility]::HtmlDecode($clean)
$lines = $clean -split "`r?`n" | ForEach-Object { $_.Trim() } | Where-Object { $_ -ne "" }

$compiler = "Unknown"
$errors = $null
$warnings = $null
$size = $null
$diagnostics = @()

foreach ($line in $lines) {
    if ($line -match 'V5\.06|ARMClang|ArmCC') {
        $compiler = $line
    }
    if ($line -match 'Program Size:\s*Code=(\d+)\s+RO-data=(\d+)\s+RW-data=(\d+)\s+ZI-data=(\d+)') {
        $code = [int]$Matches[1]
        $ro   = [int]$Matches[2]
        $rw   = [int]$Matches[3]
        $zi   = [int]$Matches[4]
        $size = [ordered]@{
            code        = $code
            ro_data     = $ro
            rw_data     = $rw
            zi_data     = $zi
            total_flash = $code + $ro + $rw
            total_ram   = $rw + $zi
        }
    }
    if ($line -match '"[^"]+"\s*-\s*(\d+)\s*Error\(s\),\s*(\d+)\s*Warning\(s\)\.') {
        $errors   = [int]$Matches[1]
        $warnings = [int]$Matches[2]
    }
    if ($line -match '^(.+?)\((\d+)\):\s*(warning|error):\s*(#[0-9A-Za-z-]+:.*)') {
        $diagnostics += [PSCustomObject]@{
            file     = $Matches[1]
            line     = [int]$Matches[2]
            severity = $Matches[3].ToUpper()
            message  = $Matches[4]
        }
    }
}

$result = [ordered]@{
    log_path    = (Resolve-Path $LogPath).Path
    compiler    = $compiler
    errors      = $errors
    warnings    = $warnings
    size        = $size
    diagnostics = $diagnostics
}

if ($SaveBaseline -ne "") {
    $result | ConvertTo-Json -Depth 5 | Set-Content -Path $SaveBaseline -Encoding UTF8
    Write-Host "Saved baseline to $SaveBaseline" -ForegroundColor Cyan
}

$baseSize = $null
if ($CompareBaseline -ne "" -and (Test-Path $CompareBaseline)) {
    $baseObj = Get-Content -Path $CompareBaseline -Raw -Encoding UTF8 | ConvertFrom-Json
    $baseSize = $baseObj.size
}

function Format-Delta([int]$curr, $baseVal) {
    if ($null -eq $baseVal) { return ("{0:N0} B" -f $curr) }
    $diff = $curr - [int]$baseVal
    $sign = if ($diff -ge 0) { "+" } else { "" }
    return ("{0:N0} B ({1}{2:N0} B)" -f $curr, $sign, $diff)
}

Write-Host "=== Keil MDK Build Summary ===" -ForegroundColor Green
Write-Host ("Compiler : {0}" -f $compiler)
Write-Host ("Result   : {0} Error(s), {1} Warning(s)" -f $errors, $warnings)

if ($null -ne $size) {
    Write-Host "`n=== Memory Footprint (Program Size) ===" -ForegroundColor Green
    Write-Host ("  Code        : {0}" -f (Format-Delta $size.code ($baseSize.code)))
    Write-Host ("  RO-data     : {0}" -f (Format-Delta $size.ro_data ($baseSize.ro_data)))
    Write-Host ("  RW-data     : {0}" -f (Format-Delta $size.rw_data ($baseSize.rw_data)))
    Write-Host ("  ZI-data     : {0}" -f (Format-Delta $size.zi_data ($baseSize.zi_data)))
    Write-Host ("  Total Flash : {0}" -f (Format-Delta $size.total_flash ($baseSize.total_flash)))
    Write-Host ("  Total RAM   : {0}" -f (Format-Delta $size.total_ram ($baseSize.total_ram)))
}

if ($diagnostics.Count -gt 0) {
    Write-Host ("`n=== Diagnostics ({0}) ===" -f $diagnostics.Count) -ForegroundColor Yellow
    foreach ($d in $diagnostics) {
        Write-Host ("  [{0}] {1}:{2} -> {3}" -f $d.severity, $d.file, $d.line, $d.message)
    }
}

if ($MapPath -ne "" -and (Test-Path $MapPath)) {
    $ramSyms = @()
    Get-Content -Path $MapPath -Encoding UTF8 | ForEach-Object {
        if ($_ -match '^\s*(0x[0-9a-fA-F]{8})\s+(0x[0-9a-fA-F]{8})\s+(Code|Data|Zero)\s+(RO|RW)\s+\d+\s+(\S+)\s+(\S+)') {
            $szBytes = [Convert]::ToInt32($Matches[2], 16)
            if ($szBytes -gt 0 -and $Matches[4] -eq "RW") {
                $ramSyms += [PSCustomObject]@{
                    Size    = $szBytes
                    Type    = "$($Matches[3])/$($Matches[4])"
                    Section = $Matches[5]
                    Object  = $Matches[6]
                }
            }
        }
    }
    $topRam = $ramSyms | Sort-Object -Property Size -Descending | Select-Object -First $Top
    if ($topRam.Count -gt 0) {
        Write-Host ("`n=== Top {0} RAM (RW/ZI) Sections in .map ===" -f $Top) -ForegroundColor Green
        foreach ($s in $topRam) {
            Write-Host ("  {0,8:N0} B | {1,-7} | {2} ({3})" -f $s.Size, $s.Type, $s.Section, $s.Object)
        }
    }
}
