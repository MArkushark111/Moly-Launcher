param([string]$BuildDirectory = 'build-release')
$ErrorActionPreference = 'Stop'
$project = Split-Path -Parent $PSScriptRoot
$source = Join-Path $project 'third_party/horizon'
if (-not (Test-Path -LiteralPath (Join-Path $source 'Cargo.toml'))) {
    throw 'Horizon source is missing. Run: git submodule update --init --recursive'
}
Push-Location $source
try {
    $branding = Join-Path $project 'patches/horizon-moly-branding.patch'
    git apply --reverse --check $branding 2>$null
    if ($LASTEXITCODE -ne 0) {
        git apply --check $branding
        if ($LASTEXITCODE) { throw 'Moly branding patch does not match the pinned engine source.' }
        git apply $branding
        if ($LASTEXITCODE) { throw 'Moly branding patch failed.' }
    }

    cargo build --locked --profile fast -j 4 -p horizon-client --bin horizon-client -p horizon-prepare --bin horizon-prepare
    if ($LASTEXITCODE) { throw 'Horizon engine build failed.' }
} finally { Pop-Location }
$output = Join-Path $BuildDirectory 'engines'
$support = Join-Path $output 'horizon-support'
New-Item -ItemType Directory -Path $support -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $source 'target/fast/horizon-client.exe') -Destination (Join-Path $output 'moly-rust.exe') -Force
Copy-Item -LiteralPath (Join-Path $source 'target/fast/horizon-prepare.exe') -Destination (Join-Path $output 'moly-rust-prepare.exe') -Force
Copy-Item -LiteralPath (Join-Path $source 'research-lock.json') -Destination $support -Force
# Only original exporter source is shipped. Jars, prepared resources and .local never leave the cache.
$relative = 'mods/horizon/src/test/java/dev/horizonlab/parity'
$exportSources = Join-Path $support $relative
New-Item -ItemType Directory -Path $exportSources -Force | Out-Null
Copy-Item -Path (Join-Path (Join-Path $source $relative) '*.java') -Destination $exportSources -Force
$compilerSources = Join-Path $support 'crates/horizon-prepare/src'
New-Item -ItemType Directory -Path $compilerSources -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $source 'crates/horizon-prepare/src/exporters.rs') -Destination $compilerSources -Force
# Precompiled original exporters make a Java runtime sufficient on a clean PC.
if (Test-Path -LiteralPath (Join-Path $source 'exporters/manifest.json')) {
    Copy-Item -LiteralPath (Join-Path $source 'exporters') -Destination $support -Recurse -Force
}

else {
    $cached = Get-ChildItem -LiteralPath (Join-Path $source '.local/exporters') -Filter manifest.json -Recurse -ErrorAction SilentlyContinue | Sort-Object LastWriteTime -Descending | Select-Object -First 1
    if ($cached) {
        $destination = Join-Path $support 'exporters'
        New-Item -ItemType Directory -Path $destination -Force | Out-Null
        Copy-Item -Path (Join-Path $cached.DirectoryName '*') -Destination $destination -Recurse -Force
    } else { throw 'Compile the original Horizon exporters before packaging: horizon-prepare compile-exporters --root third_party/horizon' }
}

python (Join-Path $PSScriptRoot 'collect-rust-notices.py') (Join-Path $output 'horizon-licenses')
if ($LASTEXITCODE) { throw 'Rust dependency notices could not be collected.' }
