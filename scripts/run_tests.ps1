# run_tests.ps1 — Unified test runner for all com0com projects
#
# Builds and runs C++ Catch2 tests and C# xUnit tests.
# Usage:
#   .\scripts\run_tests.ps1              # run all tests
#   .\scripts\run_tests.ps1 -Level unit  # unit tests only (no driver needed)
#   .\scripts\run_tests.ps1 -SkipBuild   # run without rebuilding

param(
    [ValidateSet("unit","mock","component","integration","all")]
    [string]$Level = "unit",
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $PSCommandPath
$WorkspaceRoot = Resolve-Path "$ScriptDir\.."
$MSBuild = "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe"
$SlnFile = "$WorkspaceRoot\com0com\com0com.slnx"

$totalPassed = 0
$totalFailed = 0
$failedProjects = @()

function Write-Header($text) {
    Write-Host ""
    Write-Host ("=" * 70) -ForegroundColor Cyan
    Write-Host "  $text" -ForegroundColor Cyan
    Write-Host ("=" * 70) -ForegroundColor Cyan
}

function Invoke-TestExe($exePath, $name, [string[]]$extraArgs = @()) {
    if (-not (Test-Path $exePath)) {
        Write-Host "  SKIP: $name (binary not found: $exePath)" -ForegroundColor Yellow
        return
    }

    Write-Host "  Running: $name..." -ForegroundColor White
    $result = & $exePath @extraArgs 2>&1
    $exitCode = $LASTEXITCODE

    # Parse Catch2 output for test counts
    # Success format: "All tests passed (218 assertions in 56 test cases)"
    $successMatch = $result | Select-String "All tests passed \((\d+) assertions? in (\d+) test cases?"
    # Failure format: "test cases: 55 | 51 passed | 3 failed"
    $failMatch = $result | Select-String "test cases:\s*(\d+)\s*\|\s*(\d+) passed\s*\|\s*(\d+) failed"

    if ($successMatch) {
        $assertions = $successMatch.Matches.Groups[1].Value
        $total = $successMatch.Matches.Groups[2].Value
        $script:totalPassed += [int]$total
        Write-Host "    PASS: $total tests, $assertions assertions" -ForegroundColor Green
    } elseif ($failMatch) {
        $total = $failMatch.Matches.Groups[1].Value
        $passed = $failMatch.Matches.Groups[2].Value
        $failed = $failMatch.Matches.Groups[3].Value
        $script:totalPassed += [int]$passed
        $script:totalFailed += [int]$failed

        if ($failed -eq "0") {
            Write-Host "    PASS: $passed / $total" -ForegroundColor Green
        } else {
            Write-Host "    FAIL: $passed / $total ($failed failed)" -ForegroundColor Red
            $script:failedProjects += $name
        }
    } elseif ($exitCode -ne 0) {
        Write-Host "    FAIL: exit code $exitCode (unable to parse output)" -ForegroundColor Red
        $script:failedProjects += $name
    } else {
        Write-Host "    DONE (exit code 0)" -ForegroundColor Green
    }
}

# ═══════════════════════════════════════════════════════════════════
# Build
# ═══════════════════════════════════════════════════════════════════

if (-not $SkipBuild) {
    Write-Header "Building C++ test projects"
    & $MSBuild $SlnFile /p:Configuration=Debug /p:Platform=x64 /m /v:minimal /nologo
    if ($LASTEXITCODE -ne 0) {
        Write-Host "ERROR: C++ build failed. Aborting." -ForegroundColor Red
        exit 1
    }

    Write-Header "Building C# test project"
    Push-Location "$WorkspaceRoot\com0com\setupg.Tests"
    dotnet build --no-restore 2>$null
    if ($LASTEXITCODE -ne 0) {
        Pop-Location
        Write-Host "ERROR: C# build failed. Aborting." -ForegroundColor Red
        exit 1
    }
    Pop-Location
}

# ═══════════════════════════════════════════════════════════════════
# C++ Tests (Catch2)
# ═══════════════════════════════════════════════════════════════════

$BuildDir = "$WorkspaceRoot\com0com\build\tests\Debug"

Write-Header "C++ Unit Tests (Tier 1)"
Invoke-TestExe "$BuildDir\setup_tests.exe" "setup.dll (params + comdb)"
Invoke-TestExe "$BuildDir\com2tcp_tests.exe" "com2tcp (telnet + params)"
Invoke-TestExe "$BuildDir\hub4com_tests.exe" "hub4com (hubmsg + GO/SO + ROUTINE_IS_VALID)"

# ═══════════════════════════════════════════════════════════════════
# C# Tests (xUnit)
# ═══════════════════════════════════════════════════════════════════

Write-Header "C# Unit Tests"
Push-Location "$WorkspaceRoot\com0com\setupg.Tests"
$csharpResult = dotnet test --no-restore --no-build 2>&1
Pop-Location

$csharpMatch = $csharpResult | Select-String "Passed.*Failed.*Total"
if ($csharpMatch) {
    Write-Host "  $($csharpMatch.Line.Trim())" -ForegroundColor White
    if ($csharpResult | Select-String "Failed!") {
        $script:failedProjects += "C# setupg.Tests"
    }
}

# ═══════════════════════════════════════════════════════════════════
# Summary
# ═══════════════════════════════════════════════════════════════════

Write-Header "Summary"
Write-Host "  C++ tests passed: $totalPassed" -ForegroundColor $(if ($totalFailed -eq 0) { "Green" } else { "Red" })
Write-Host "  C++ tests failed: $totalFailed" -ForegroundColor $(if ($totalFailed -eq 0) { "Green" } else { "Red" })

if ($failedProjects.Count -gt 0) {
    Write-Host ""
    Write-Host "  Failed projects:" -ForegroundColor Red
    foreach ($proj in $failedProjects) {
        Write-Host "    - $proj" -ForegroundColor Red
    }
    exit 1
} else {
    Write-Host ""
    Write-Host "  All tests passed." -ForegroundColor Green
    exit 0
}
