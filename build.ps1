# Build script for SFML MineSweeper project
param(
    [switch]$d,  # Debug build
    [switch]$r   # Release build
)

$ErrorActionPreference = "Stop"

# Determine build type
$buildType = "Debug"

if ($r) {
    $buildType = "Release"
    Write-Host "Building in RELEASE mode..." -ForegroundColor Green
}
elseif ($d) {
    Write-Host "Building in DEBUG mode..." -ForegroundColor Green
}
else {
    Write-Host "WARNING: No build type specified. Defaulting to DEBUG mode." -ForegroundColor Yellow
    Write-Host "Use -d for Debug or -r for Release builds." -ForegroundColor Yellow
    Write-Host "Building in DEBUG mode..." -ForegroundColor Green
}

Write-Host "Current directory: $PWD" -ForegroundColor Cyan

# Create build directory if it doesn't exist
if (-not (Test-Path "build")) {
    Write-Host "Creating build directory..." -ForegroundColor Yellow
    New-Item -ItemType Directory -Path "build" | Out-Null
}

# Change to build directory
Set-Location build

try {
    # Run cmake configuration
    Write-Host "Running CMake configuration..." -ForegroundColor Yellow
    cmake ..
    if ($LASTEXITCODE -ne 0) {
        throw "CMake configuration failed"
    }

    # Build the project
    Write-Host "Building project in $buildType mode..." -ForegroundColor Yellow
    cmake --build . --config $buildType
    if ($LASTEXITCODE -ne 0) {
        throw "Build failed"
    }

    Write-Host "Build completed successfully in $buildType mode!" -ForegroundColor Green
}
catch {
    Write-Host "ERROR: $_" -ForegroundColor Red
    Set-Location ..
    exit 1
}
finally {
    Set-Location ..
}

