param([switch]$Demo)
$ErrorActionPreference = 'Stop'
$application = Join-Path $PSScriptRoot 'build-msvc\dist\LabelStudioDesktop.exe'
if (-not (Test-Path -LiteralPath $application)) {
    & (Join-Path $PSScriptRoot 'build.ps1') -Test -Deploy
}
if ($Demo) { & $application --demo } else { & $application }
