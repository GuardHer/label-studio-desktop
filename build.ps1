param(
    [string]$QtRoot = 'E:\Qt\Qt6.12\6.12.0\msvc2022_64',
    [string]$VisualStudio = '',
    [switch]$Test,
    [switch]$Deploy,
    [switch]$Run
)
$ErrorActionPreference = 'Stop'
$env:VSLANG = '1033'
if (-not $VisualStudio) {
    $locator = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (Test-Path -LiteralPath $locator) {
        $VisualStudio = (& $locator -latest -prerelease -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath | Select-Object -First 1)
    }
}
if (-not $VisualStudio -or -not (Test-Path -LiteralPath "$VisualStudio\Common7\Tools\VsDevCmd.bat")) {
    throw 'MSVC not found. Pass -VisualStudio with your Visual Studio installation path.'
}
if (-not (Test-Path -LiteralPath "$QtRoot\lib\cmake\Qt6\Qt6Config.cmake")) { throw 'QtRoot must point to an installed MSVC x64 Qt kit.' }
$envProcessInfo = New-Object System.Diagnostics.ProcessStartInfo
$envProcessInfo.FileName = $env:ComSpec
$envProcessInfo.Arguments = '/d /s /c ""' + $VisualStudio + '\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul && set"'
$envProcessInfo.UseShellExecute = $false
$envProcessInfo.CreateNoWindow = $true
$envProcessInfo.RedirectStandardOutput = $true
$envProcess = [System.Diagnostics.Process]::Start($envProcessInfo)
$envLines = $envProcess.StandardOutput.ReadToEnd() -split "`r?`n"
$envProcess.WaitForExit()
if ($envProcess.ExitCode -ne 0) { throw 'Could not initialize MSVC environment.' }
foreach ($entry in $envLines) {
    if ($entry -match '^([^=]+)=(.*)$') { [Environment]::SetEnvironmentVariable($Matches[1], $Matches[2], 'Process') }
}
$env:VSLANG = '1033'
$cmake = "$VisualStudio\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
$ninja = "$VisualStudio\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"
if (-not (Test-Path -LiteralPath $cmake)) { throw 'Install the Visual Studio C++ CMake tools component.' }
$buildDir = Join-Path $PSScriptRoot 'build-msvc'
$env:PATH = "$QtRoot\bin;" + $env:PATH
New-Item -ItemType Directory -Path $buildDir -Force | Out-Null
# Discover the actual /showIncludes language: some MSVC installations only have
# Chinese compiler resources. A wrong prefix prevents Ninja tracking headers.
$includeProbe = Join-Path $buildDir 'include-language.cpp'
Set-Content -LiteralPath $includeProbe -Encoding ascii -Value '#include <stddef.h>'
$probeOutput = & cl.exe /nologo /showIncludes /Zs $includeProbe 2>&1
if ($LASTEXITCODE -ne 0) { throw 'MSVC include scanner probe failed.' }
$includePrefix = $null
foreach ($line in $probeOutput) {
    if ("$line" -match '^(.+?:)\s+[A-Za-z]:[\\/].*[\\/]stddef\.h\s*$') { $includePrefix = $Matches[1]; break }
}
if (-not $includePrefix) { throw 'Could not detect the MSVC /showIncludes prefix.' }
$rulesOverride = Join-Path $buildDir 'include-language.cmake'
Set-Content -LiteralPath $rulesOverride -Encoding utf8 -Value ('set(CMAKE_CL_SHOWINCLUDES_PREFIX "' + $includePrefix + '")')
& $cmake -S $PSScriptRoot -B $buildDir -G Ninja "-DCMAKE_MAKE_PROGRAM=$ninja" "-DCMAKE_PREFIX_PATH=$QtRoot" -DCMAKE_BUILD_TYPE=Release "-DCMAKE_USER_MAKE_RULES_OVERRIDE_CXX=$rulesOverride"
if ($LASTEXITCODE -ne 0) { throw 'CMake configuration failed.' }
& $cmake --build $buildDir --parallel 4
if ($LASTEXITCODE -ne 0) { throw 'MSVC build failed.' }
if ($Test) {
    $testLog = Join-Path $buildDir 'test-results.log'
    & (Join-Path (Split-Path $cmake) 'ctest.exe') --test-dir $buildDir --output-on-failure --output-log $testLog
    if ($LASTEXITCODE -ne 0) { throw 'Tests failed.' }
}
if ($Deploy) {
    $dist = Join-Path $buildDir 'dist'
    New-Item -ItemType Directory -Path $dist -Force | Out-Null
    Copy-Item -LiteralPath "$buildDir\LabelStudioDesktop.exe" -Destination $dist
    & "$QtRoot\bin\windeployqt.exe" --release --qmldir "$PSScriptRoot\qml" --no-translations --dir $dist "$dist\LabelStudioDesktop.exe"
    if ($LASTEXITCODE -ne 0) { throw 'Qt deployment failed.' }
    $licenseDir = Join-Path $dist 'licenses'
    New-Item -ItemType Directory -Path $licenseDir -Force | Out-Null
    foreach ($licenseFile in @('README.md', 'LABEL-STUDIO-LICENSE', 'LABEL-STUDIO-NOTICE', 'OFL.txt')) {
        Copy-Item -LiteralPath (Join-Path "$PSScriptRoot\assets" $licenseFile) -Destination $licenseDir
    }
    Write-Output "Desktop application: $dist\LabelStudioDesktop.exe"
}
if ($Run) { & "$buildDir\LabelStudioDesktop.exe" }
