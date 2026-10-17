param([string]$BackendRoot = 'E:\projects\QML\label-studio')
$ErrorActionPreference = 'Stop'
$desktopRoot = Split-Path $PSScriptRoot
$env:PATH = 'E:\Qt\Qt6.12\6.12.0\msvc2022_64\bin;' + $env:PATH
$env:QT_QPA_PLATFORM = 'offscreen'
$env:QT_QUICK_BACKEND = 'software'
$env:QT_QPA_FONTDIR = Join-Path $desktopRoot 'build-msvc/test-fonts'
$env:QML_ACCOUNT_LIVE_EMAIL = 'qml-account-qa-' + [Guid]::NewGuid().ToString('N') + '@example.invalid'
$env:QML_ACCOUNT_LIVE_PASSWORD = 'Qa-' + [Guid]::NewGuid().ToString('N') + '!'
$env:QML_ACCOUNT_ISOLATED = '1'
$env:QML_ACCOUNT_SCREENSHOTS = Join-Path $desktopRoot 'build-msvc/account-screenshots'
$python = Join-Path $BackendRoot '.venv/Scripts/python.exe'
$fixture = Join-Path $PSScriptRoot 'account_live_fixture.py'
try {
    & $python $fixture setup $BackendRoot
    if ($LASTEXITCODE -ne 0) { throw 'Account fixture setup failed.' }
    & (Join-Path $desktopRoot 'build-msvc/tst_live.exe') nativeAccountRoundTrip -o (Join-Path $desktopRoot 'build-msvc/account-live-results.txt,txt')
    if ($LASTEXITCODE -ne 0) { throw 'Native account live test failed. See build-msvc/account-live-results.txt.' }
} finally {
    & $python $fixture cleanup $BackendRoot
    Remove-Item Env:QML_ACCOUNT_LIVE_EMAIL, Env:QML_ACCOUNT_LIVE_PASSWORD, Env:QML_ACCOUNT_ISOLATED -ErrorAction SilentlyContinue
}
