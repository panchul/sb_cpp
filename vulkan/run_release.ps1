$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$buildDir = Join-Path $scriptDir "build/release"
$executable = Join-Path $buildDir "vulkan_sandbox.exe"

if (-not (Test-Path $executable)) {
    Write-Error "Release executable not found: $executable. Run ./build_release.ps1 first."
}

& $executable
