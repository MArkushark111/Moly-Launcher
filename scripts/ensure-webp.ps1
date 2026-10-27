param([string]$BuildDirectory = 'build')
$ErrorActionPreference = 'Stop'

# Add the official Qt 6.11.2 MinGW WebP decoder beside this build only.
# This does not change the global Qt installation or run a downloaded installer.
$projectDirectory = Split-Path -Parent $PSScriptRoot
$buildPath = [IO.Path]::GetFullPath((Join-Path $projectDirectory $BuildDirectory))
$toolsPath = Join-Path $buildPath 'tools'
$extractPath = Join-Path $toolsPath 'qtimageformats'
$pluginDirectory = Join-Path $buildPath 'imageformats'
$packageName = 'qt.qt6.6112.addons.qtimageformats.win64_mingw'
$packageVersion = '6.11.2-0-202608131017'
$archiveName = 'qtimageformats-Windows-Windows_11_24H2-Mingw-Windows-Windows_11_24H2-X86_64.7z'
$repository = 'https://download.qt.io/online/qtsdkrepository/windows_x86/desktop/qt6_6112/qt6_6112_mingw'
$archiveUrl = "$repository/$packageName/$packageVersion$archiveName"
$archivePath = Join-Path $toolsPath $archiveName
$expectedSha1 = 'c482247c13796b227d94faff0e5f40b1756fb228'

New-Item -ItemType Directory -Path $toolsPath, $extractPath, $pluginDirectory -Force | Out-Null

$checksumResponse = Invoke-WebRequest -Uri ($archiveUrl + '.sha1') -UseBasicParsing
$remoteSha1 = if ($checksumResponse.Content -is [byte[]]) {
    [Text.Encoding]::ASCII.GetString($checksumResponse.Content).Trim()
} else {
    ([string]$checksumResponse.Content).Trim()
}
if ($remoteSha1.ToLowerInvariant() -ne $expectedSha1) {
    throw 'The official Qt imageformats archive differs from the pinned 6.11.2 build.'
}

$cachedHash = if (Test-Path -LiteralPath $archivePath) {
    (Get-FileHash -LiteralPath $archivePath -Algorithm SHA1).Hash.ToLowerInvariant()
} else { '' }
if ($cachedHash -ne $expectedSha1) {
    Invoke-WebRequest -Uri $archiveUrl -OutFile $archivePath
}
if ((Get-FileHash -LiteralPath $archivePath -Algorithm SHA1).Hash.ToLowerInvariant() -ne $expectedSha1) {
    throw 'Qt imageformats archive checksum verification failed.'
}

$tarPath = Join-Path $env:SystemRoot 'System32\tar.exe'
if (-not (Test-Path -LiteralPath $tarPath)) {
    throw 'Windows tar.exe is required to extract the official Qt archive.'
}
# Extract only the required member; no archive-supplied executable is launched.
& $tarPath -xf $archivePath -C $extractPath 'plugins/imageformats/qwebp.dll'
if ($LASTEXITCODE) { throw 'Qt WebP plugin extraction failed.' }
$pluginPath = Join-Path $extractPath 'plugins\imageformats\qwebp.dll'
if (-not (Test-Path -LiteralPath $pluginPath)) { throw 'The archive did not contain qwebp.dll.' }
Copy-Item -LiteralPath $pluginPath -Destination (Join-Path $pluginDirectory 'qwebp.dll') -Force
Write-Output "Qt 6.11.2 WebP plugin ready: $pluginDirectory\qwebp.dll"
