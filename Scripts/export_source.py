#!/usr/bin/env python3
"""Export source files using the repository's ignore rules, never local licensed dependencies."""
import argparse
from pathlib import Path
import subprocess
import zipfile

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output', type=Path, help='Destination ZIP outside the repository')
parser.add_argument('--list', action='store_true', help='Preview the export without writing a ZIP')
args = parser.parse_args()
paths = subprocess.check_output(['git', '-C', str(ROOT), 'ls-files', '--cached', '--others', '--exclude-standard', '-z']).decode().split('\0')
paths = sorted({p for p in paths if p})
ignored = subprocess.run(['git', '-C', str(ROOT), 'check-ignore', '--no-index', '--stdin', '-z'], input=('\0'.join(paths) + '\0').encode(), capture_output=True)
if ignored.returncode not in (0, 1):
    raise RuntimeError(ignored.stderr.decode())
excluded = set(ignored.stdout.decode().split('\0'))
files = [p for p in paths if p not in excluded and (ROOT / p).is_file()]
for p in files:
    if (ROOT / p).is_symlink():
        raise RuntimeError('Review symlink before exporting: ' + p)
if args.list:
    for p in files:
        print(p)
    print(f'\n{len(files)} files; {sum((ROOT / p).stat().st_size for p in files):,} source bytes')
else:
    if args.output is None:
        parser.error('Use --list first, or supply --output /path/outside/repository.zip')
    output = args.output.resolve()
    if output == ROOT or ROOT in output.parents:
        parser.error('Write the source ZIP outside the working repository.')
    if output.exists():
        parser.error('The output already exists; choose a new filename.')
    with zipfile.ZipFile(output, 'x', compression=zipfile.ZIP_DEFLATED) as archive:
        for p in files:
            archive.write(ROOT / p, 'paradise-heat/' + p)
    print('Source ZIP written:', output)
