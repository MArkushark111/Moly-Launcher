"""Collect license texts for the source-built Rust engine into a build directory."""
import json
from pathlib import Path
import shutil
import subprocess
import sys

source = Path(__file__).resolve().parent.parent / 'third_party' / 'horizon'
destination = Path(sys.argv[1]).resolve()
destination.mkdir(parents=True, exist_ok=True)
metadata = json.loads(subprocess.check_output([
    'cargo', 'metadata', '--locked', '--format-version', '1',
    '--filter-platform', 'x86_64-pc-windows-msvc',
    '--manifest-path', str(source / 'Cargo.toml')], text=True))
notices = ['Rust engine dependency notices', '']
for package in sorted(metadata['packages'], key=lambda value: value['name']):
    notices.append(f"{package['name']} {package['version']}: {package.get('license') or 'See source'}")
    folder = Path(package['manifest_path']).parent
    for file in folder.iterdir():
        if file.is_file() and file.name.upper().startswith(('LICENSE', 'COPYING', 'NOTICE')):
            target = destination / (package['name'] + '-' + package['version']) / file.name
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(file, target)
tracked = subprocess.check_output(['git', '-C', str(source), 'ls-files'], text=True).splitlines()
for name in tracked:
    if Path(name).name.upper().startswith(('LICENSE', 'OFL', 'NOTICE', 'THIRD_PARTY_NOTICES')):
        target = destination / 'horizon-source' / name
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source / name, target)
(destination / 'DEPENDENCIES.txt').write_text('\n'.join(notices) + '\n', encoding='utf-8')
