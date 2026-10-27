"""Create one non-installing portable EXE from a deployed directory.
Only the pinned, hash-verified official LZMA SDK SFX is accepted.
"""
import argparse
import hashlib
from pathlib import Path
import shutil
import subprocess
import tempfile
import urllib.request

SDK_URL = 'https://github.com/ip7z/7zip/releases/download/26.05/lzma2605.7z'
SDK_SHA256 = '668fda469b7f584f246008cf28359c66ef98d65fafb3b22a52672eebe6970c28'


def package(stage, output, sdk=None, sevenzip='7z'):
    stage, output = Path(stage).resolve(), Path(output).resolve()
    for name in ('qsstv.exe', 'portable-launcher.exe'):
        if not (stage / name).is_file():
            raise ValueError(f'Missing payload: {name}')
    if output.exists() or output.is_symlink() or stage in output.parents:
        raise ValueError('Output must be new and outside the payload directory')
    with tempfile.TemporaryDirectory(prefix='qsstv-portable-') as temporary:
        temporary = Path(temporary)
        if sdk is None:
            sdk = temporary / 'sdk.7z'
            with urllib.request.urlopen(SDK_URL, timeout=60) as response, sdk.open('wb') as target:
                shutil.copyfileobj(response, target)
        sdk = Path(sdk).resolve()
        if hashlib.sha256(sdk.read_bytes()).hexdigest() != SDK_SHA256:
            raise ValueError('Official LZMA SDK SHA-256 mismatch')
        subprocess.run([sevenzip, 'x', '-y', '-o' + str(temporary / 'sdk'), str(sdk),
                        'bin/7zSD.sfx', 'DOC/lzma-sdk.txt'], check=True)
        notice = stage / 'licenses' / 'lzma-sdk.txt'
        notice.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(temporary / 'sdk/DOC/lzma-sdk.txt', notice)
        archive = temporary / 'payload.7z'
        subprocess.run([sevenzip, 'a', '-t7z', '-m0=LZMA2', '-mx=7', str(archive), '.'], cwd=stage, check=True)
        subprocess.run([sevenzip, 't', str(archive)], check=True)
        config = (b';!@Install@!UTF-8!\nTitle="QSSTV portable"\nProgress="yes"\n'
                  b'RunProgram="portable-launcher.exe"\n;!@InstallEnd@!\n')
        output.parent.mkdir(parents=True, exist_ok=True)
        with output.open('xb') as target:
            target.write((temporary / 'sdk/bin/7zSD.sfx').read_bytes())
            target.write(config)
            with archive.open('rb') as source:
                shutil.copyfileobj(source, target)
        subprocess.run([sevenzip, 't', str(output)], check=True)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('stage', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('--sdk', type=Path, help='Previously downloaded official SDK (hash checked)')
    parser.add_argument('--sevenzip', default='7z')
    args = parser.parse_args()
    package(args.stage, args.output, args.sdk, args.sevenzip)
