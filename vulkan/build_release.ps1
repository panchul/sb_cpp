$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$buildDir = Join-Path $scriptDir "build/release"

New-Item -ItemType Directory -Force -Path $buildDir | Out-Null

cmake -S $scriptDir -B $buildDir -DCMAKE_BUILD_TYPE=Release
cmake --build $buildDir --config Release --parallel
