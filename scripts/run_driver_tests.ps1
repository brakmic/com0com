# run_driver_tests.ps1 — Run Tier 4 null-modem tests as Administrator
# Usage: Right-click PowerShell → Run as Administrator, then:
#   .\scripts\run_driver_tests.ps1

$ErrorActionPreference = "Stop"

$TestExe = "$PSScriptRoot\..\com0com\build\tests\Debug\setup_tests.exe"

if (-not (Test-Path $TestExe)) {
    Write-Host "ERROR: $TestExe not found. Build first." -ForegroundColor Red
    exit 1
}

Write-Host "Running Tier 4 null-modem tests..." -ForegroundColor Cyan
Write-Host ""

# Run tests in order to avoid port conflicts (exclusive access)
& $TestExe "[driver]" --order lex -s 2>&1

Write-Host ""
Write-Host "Exit code: $LASTEXITCODE" -ForegroundColor $(if ($LASTEXITCODE -eq 0) { "Green" } else { "Red" })
exit $LASTEXITCODE
