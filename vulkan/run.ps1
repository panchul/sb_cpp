param(
    [ValidateSet("debug", "release")]
    [string]$Mode = "debug"
)

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path

if ($Mode -eq "debug") {
    & (Join-Path $scriptDir "run_debug.ps1")
} else {
    & (Join-Path $scriptDir "run_release.ps1")
}
