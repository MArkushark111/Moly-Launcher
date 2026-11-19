param(
    [Parameter(Mandatory)][ValidateSet('Standard','Local')][string]$Edition,
    [Parameter(Mandatory)][string]$BuildDirectory,
    [string]$EngineDirectory = 'build-release/engines'
)
$ErrorActionPreference = 'Stop'
$project = Split-Path -Parent $PSScriptRoot
Push-Location $project
try {
    $qt = 'C:\Qt\6.11.2\mingw_64\bin'
    $env:PATH = 'C:\Qt\Tools\mingw1310_64\bin;' + $qt + ';' + $env:PATH
    $build = (Resolve-Path -LiteralPath $BuildDirectory).Path
    $cache = Get-Content -LiteralPath (Join-Path $build 'CMakeCache.txt')
    if ($cache -notcontains 'MOLY_PUBLIC_BUILD:BOOL=ON') { throw 'Configure with MOLY_PUBLIC_BUILD=ON first.' }
    $local = $Edition -eq 'Local'
    if ($local -ne ($cache -contains 'MOLY_LOCAL_EDITION:BOOL=ON')) { throw 'Build edition does not match.' }
    $stage = New-Item -ItemType Directory -Path "dist/public/$Edition-$([guid]::NewGuid().ToString('N'))"
    $exe = if ($local) { 'MolyLocalLauncher.exe' } else { 'MolyLauncher.exe' }
    Copy-Item -LiteralPath (Join-Path $build $exe) -Destination $stage.FullName
    & "$qt\windeployqt.exe" --release --no-translations --qmldir "$project\qml" (Join-Path $stage.FullName $exe)
    if ($LASTEXITCODE) { throw 'Qt deployment failed.' }

    $licenseDir = New-Item -ItemType Directory -Path (Join-Path $stage.FullName 'licenses')
    Copy-Item licenses/Inter-OFL.txt, licenses/Qt-Copyright.txt, licenses/Qt-LICENSE.txt, licenses/libwebp-COPYING.txt -Destination $licenseDir.FullName
    Copy-Item README.md, CHANGELOG.md, THIRD_PARTY.md -Destination $stage.FullName
    if (!$local) {
        $engineOut = New-Item -ItemType Directory -Path (Join-Path $stage.FullName 'engines')
        foreach ($binary in @('moly-rust.exe','moly-rust-prepare.exe')) {
            Copy-Item -LiteralPath (Join-Path $EngineDirectory $binary) -Destination $engineOut.FullName
        }
        $support = Join-Path (Resolve-Path -LiteralPath $EngineDirectory).Path 'horizon-support'
        $targetSupport = Join-Path $engineOut.FullName 'horizon-support'
        foreach ($file in (Get-ChildItem -LiteralPath $support -Recurse -File)) {
            if ($file.Extension -notin '.class','.java','.json','.rs') { continue }
            $relative = [IO.Path]::GetRelativePath($support,$file.FullName)
            $target = Join-Path $targetSupport $relative
            New-Item -ItemType Directory -Force -Path (Split-Path -Parent $target) | Out-Null
            Copy-Item -LiteralPath $file.FullName -Destination $target
        }
        $rustLicenses = New-Item -ItemType Directory -Path (Join-Path $licenseDir.FullName 'horizon')
        Copy-Item third_party/horizon/LICENSE, third_party/horizon/THIRD_PARTY_NOTICES.md -Destination $rustLicenses.FullName
        Copy-Item (Join-Path $EngineDirectory 'horizon-licenses') -Destination (Join-Path $rustLicenses.FullName 'dependencies') -Recurse
        Copy-Item assets/rust-shaders -Destination $stage.FullName -Recurse
        Copy-Item rust-engine/README.md -Destination (Join-Path $stage.FullName 'Rust Edition.md')
    }
    $files = Get-ChildItem -LiteralPath $stage.FullName -Recurse -File
    foreach ($file in $files) {
        $relative = [IO.Path]::GetRelativePath($stage.FullName,$file.FullName)
        if ($relative -match '(^|[\\/])(\.local|local-library|cache|runtimes|accounts\.json|settings\.json|saves|local-secrets\.cmake)([\\/]|$)' -or
            $file.Extension -in '.jar','.zip','.mlypack','.mrpack','.log','.bin','.pem','.key') {
            throw "Unexpected payload in public installer: $relative"
        }
    }
    $inno = Join-Path $env:LOCALAPPDATA 'Programs/Inno Setup 7/ISCC.exe'
    $recipe = if ($local) { 'packaging/moly-local-public.iss' } else { 'packaging/moly.iss' }
    $output = New-Item -ItemType Directory -Force -Path 'dist/public/installers'
    & $inno "/DAppSource=$($stage.FullName)" "/O$($output.FullName)" $recipe
    if ($LASTEXITCODE) { throw 'Building the installer failed.' }
    Set-Content -LiteralPath "dist/public/$Edition-stage.txt" -Value $stage.FullName
} finally { Pop-Location }
