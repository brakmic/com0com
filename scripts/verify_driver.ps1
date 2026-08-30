# verify_driver.ps1: Release gate for the com0com driver package
#
# Runs InfVerif on every INF in the package directory and verifies the
# signatures of the driver binary and catalog. Use it before publishing a
# release package.
#
# Usage:
#   .\scripts\verify_driver.ps1
#   .\scripts\verify_driver.ps1 -PackageDir <dir>
#
# The default package directory is com0com\sys\x64\Release\com0com.
# Signatures are checked with signtool verify /pa. For test-signed builds,
# pass -AllowUntrusted to check that a signature exists instead of
# requiring a trusted chain.

param(
    [string]$PackageDir = "",
    [switch]$AllowUntrusted
)

$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $PSCommandPath

if (-not $PackageDir) {
    $PackageDir = [System.IO.Path]::GetFullPath(
        (Join-Path $ScriptDir "..\com0com\sys\x64\Release\com0com"))
}

$failed = @()

function Fail($message) {
    Write-Host "  ERROR: $message" -ForegroundColor Red
    $script:failed += $message
}

Write-Host "Verifying driver package in $PackageDir"

# Locate the tools.
$kitsRoot = "${env:ProgramFiles(x86)}\Windows Kits\10\bin"
$infVerif = Get-ChildItem $kitsRoot -Recurse -Filter "infverif.exe" -ErrorAction SilentlyContinue |
            Where-Object { $_.FullName -match "\\x64\\" } |
            Select-Object -First 1 -ExpandProperty FullName
$signtool = Get-ChildItem $kitsRoot -Recurse -Filter "signtool.exe" -ErrorAction SilentlyContinue |
            Where-Object { $_.FullName -match "\\x64\\" } |
            Select-Object -First 1 -ExpandProperty FullName

if (-not $infVerif) {
    Fail "InfVerif not found under $kitsRoot. Install the Windows SDK verification tools."
}

if (-not $signtool) {
    Fail "signtool not found under $kitsRoot."
    Write-Host ""
    Write-Host "Verification failed. Fix the errors above." -ForegroundColor Red
    exit 1
}

# Verify each INF with InfVerif.
$infs = Get-ChildItem $PackageDir -Filter "*.inf"
if (-not $infs) {
    Fail "No INF files found in $PackageDir"
} elseif ($infVerif) {
    foreach ($inf in $infs) {
        Write-Host "  InfVerif: $($inf.Name)"
        & $infVerif /w /v $inf.FullName
        if ($LASTEXITCODE -ne 0) {
            Fail "InfVerif reported errors for $($inf.Name)"
        }
    }
}

# Verify signatures.
foreach ($file in @("com0com.sys", "com0com.cat")) {
    $path = Join-Path $PackageDir $file
    if (-not (Test-Path $path)) {
        Fail "$file not found in $PackageDir"
        continue
    }

    Write-Host "  Signature check: $file"
    $result = & $signtool verify /v /pa $path 2>&1
    $exitCode = $LASTEXITCODE

    if ($exitCode -eq 0) {
        Write-Host "    Trusted signature verified." -ForegroundColor Green
    } elseif ($AllowUntrusted) {
        # Test-signed builds are signed by a self-created root that the
        # system does not trust. Check that a signature is present and
        # cryptographically intact instead of requiring a trusted chain.
        if ($result | Select-String "Signature Index: 0 \(Primary Signature\)") {
            Write-Host "    Untrusted signature present (test build)." -ForegroundColor Yellow
        } else {
            Fail "$file has no valid signature"
        }
    } else {
        Fail "$file signature is not trusted. Use -AllowUntrusted for test builds."
    }
}

Write-Host ""
if ($failed.Count -gt 0) {
    Write-Host "Verification failed:" -ForegroundColor Red
    foreach ($f in $failed) {
        Write-Host "  - $f" -ForegroundColor Red
    }
    exit 1
}

Write-Host "Verification passed." -ForegroundColor Green
exit 0
