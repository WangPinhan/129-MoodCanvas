# MoodCanvas cloud server - run in a dedicated PowerShell window
$ErrorActionPreference = "Stop"
Set-Location $PSScriptRoot

Write-Host ""
Write-Host "=== MoodCanvas Cloud Server ===" -ForegroundColor Cyan
Write-Host "[1/2] Installing Python packages..."
python -m pip install -r requirements.txt -q

$port = 8765
$inUse = Get-NetTCPConnection -LocalPort $port -State Listen -ErrorAction SilentlyContinue
if ($null -ne $inUse) {
    Write-Host ""
    Write-Host "Port $port is already in use. Server may already be running." -ForegroundColor Yellow
    Write-Host "Test in browser: http://127.0.0.1:$port/api/health"
    Read-Host "Press Enter to exit"
    exit 0
}

Write-Host "[2/2] Starting http://127.0.0.1:$port"
Write-Host "Keep this window open. Press Ctrl+C to stop the server." -ForegroundColor Green
Write-Host ""
python -m uvicorn main:app --host 127.0.0.1 --port $port
