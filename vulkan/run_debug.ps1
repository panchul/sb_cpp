$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$buildDir = Join-Path $scriptDir "build/debug"
$executable = Join-Path $buildDir "vulkan_sandbox.exe"

if (-not (Test-Path $executable)) {
    Write-Error "Debug executable not found: $executable. Run ./build_debug.ps1 first."
}

& $executable
