param([switch]$Package, [string]$InnoCompiler, [string]$BuildDirectory = 'build')
$ErrorActionPreference = 'Stop'
$project = Split-Path -Parent $PSScriptRoot
Push-Location $project
try {
    $env:PATH = 'C:\Qt\Tools\mingw1310_64\bin;C:\Qt\6.11.2\mingw_64\bin;' + $env:PATH
    cmake --preset windows-qt-mingw -B $BuildDirectory
    if ($LASTEXITCODE) { throw 'CMake configuration failed.' }
    cmake --build $BuildDirectory --parallel 6
    if ($LASTEXITCODE) { throw 'Build failed.' }
    if (-not (Test-Path -LiteralPath (Join-Path $BuildDirectory 'imageformats/qwebp.dll'))) {
        & ./scripts/ensure-webp.ps1 -BuildDirectory $BuildDirectory
    }
    ctest --test-dir $BuildDirectory --output-on-failure
    if ($LASTEXITCODE) { throw 'Tests failed.' }
    if ($Package) {
        $packageDirectory = "dist/MolyLauncher-0.8.0"
        cmake --install $BuildDirectory --prefix $packageDirectory
        if ($LASTEXITCODE) { throw 'Deployment failed.' }
        Copy-Item README.md, THIRD_PARTY.md, CHANGELOG.md -Destination $packageDirectory
        Copy-Item -Recurse licenses -Destination $packageDirectory -Force
        $guideDirectory = Join-Path $packageDirectory "docs"
        New-Item -ItemType Directory -Path $guideDirectory -Force | Out-Null
        Copy-Item docs/MICROSOFT_SIGN_IN.md, docs/TESTING.md, docs/PORTABLE_PACKS.md -Destination $guideDirectory
        $portableFiles = (Get-ChildItem $packageDirectory | Where-Object Name -ne 'bin').FullName
        Compress-Archive -Path $portableFiles -DestinationPath dist/MolyLauncher-0.8.0-Windows-x64.zip -Force
        if ($InnoCompiler) {
            & $InnoCompiler packaging/moly.iss
            if ($LASTEXITCODE) { throw 'Installer build failed.' }
        }
    }
} finally { Pop-Location }
