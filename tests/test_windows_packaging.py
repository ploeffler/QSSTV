"""Device-free packaging checks; optional real SDK archive via QSSTV_LZMA_SDK."""
import importlib.util
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
import yaml

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('portable', ROOT / 'scripts/package-windows-portable.py')
portable = importlib.util.module_from_spec(spec)
spec.loader.exec_module(portable)


class WindowsPackagingTests(unittest.TestCase):
    def fixture(self, root, missing=False):
        stage, prefix, system = root / 'payload', root / 'mingw', root / 'system'
        (stage / 'audio').mkdir(parents=True)
        (prefix / 'bin').mkdir(parents=True)
        system.mkdir()
        (stage / 'qsstv.exe').write_text('app')
        (stage / 'audio/plugin.dll').write_text('plugin')
        (prefix / 'bin/first.dll').write_text('first')
        (prefix / 'bin/second.dll').write_text('second')
        (prefix / 'bin/audio.dll').write_text('audio')
        (system / 'kernel32.dll').write_text('system')
        mock = root / 'objdump'
        mock.write_text('''#!/bin/bash
case "$(basename "$2")" in
 qsstv.exe) printf ' DLL Name: FIRST.dll\\n DLL Name: KERNEL32.dll\\n';;
 first.dll) echo ' DLL Name: second.dll';;
 second.dll) echo ' DLL Name: first.dll';;
 plugin.dll) echo ' DLL Name: audio.dll';;
 audio.dll) echo ' DLL Name: api-ms-win-core-test.dll';;
esac
''')
        mock.chmod(0o755)
        if missing:
            (prefix / 'bin/second.dll').unlink()
        return stage, prefix, system, mock

    def test_recursive_dependencies_and_plugins(self):
        with tempfile.TemporaryDirectory() as temp:
            stage, prefix, system, mock = self.fixture(Path(temp))
            subprocess.run(['bash', str(ROOT / 'scripts/collect-windows-dlls.sh'), str(stage), str(prefix), str(system)],
                           env=dict(os.environ, OBJDUMP=str(mock)), check=True)
            self.assertEqual((stage / 'audio.dll').read_text(), 'audio')
            self.assertEqual((stage / 'first.dll').read_text(), 'first')
            self.assertEqual((stage / 'second.dll').read_text(), 'second')
            self.assertFalse((stage / 'kernel32.dll').exists())

    def test_missing_dependency_fails(self):
        with tempfile.TemporaryDirectory() as temp:
            stage, prefix, system, mock = self.fixture(Path(temp), missing=True)
            result = subprocess.run(['bash', str(ROOT / 'scripts/collect-windows-dlls.sh'), str(stage), str(prefix), str(system)],
                                    env=dict(os.environ, OBJDUMP=str(mock)), capture_output=True, text=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn('Missing runtime dependency second.dll', result.stderr)

    def test_sdk_hash_rejected_and_existing_output_preserved(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp);stage = root / 'payload';stage.mkdir()
            for name in ('qsstv.exe', 'portable-launcher.exe'):
                (stage / name).write_bytes(b'MZ fixture')
            sdk = root / 'bad-sdk';sdk.write_bytes(b'wrong')
            output = root / 'portable.exe'
            with self.assertRaisesRegex(ValueError, 'SHA-256'):
                portable.package(stage, output, sdk)
            self.assertFalse(output.exists())
            output.write_bytes(b'keep')
            with self.assertRaisesRegex(ValueError, 'Output must be new'):
                portable.package(stage, output, sdk)
            self.assertEqual(output.read_bytes(), b'keep')

    @unittest.skipUnless(os.environ.get('QSSTV_LZMA_SDK'), 'Set QSSTV_LZMA_SDK for actual SFX assembly')
    def test_actual_sfx_payload_roundtrip(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp);stage = root / 'payload';(stage / 'platforms').mkdir(parents=True)
            files = {'qsstv.exe': b'MZ application fixture', 'portable-launcher.exe': b'MZ launcher fixture',
                     'platforms/qwindows.dll': b'plugin bytes', 'qt.conf': b'[Paths]\nPlugins=.\n'}
            for name, content in files.items():
                (stage / name).write_bytes(content)
            output = root / 'QSSTV portable.exe'
            portable.package(stage, output, Path(os.environ['QSSTV_LZMA_SDK']))
            self.assertTrue(output.read_bytes().startswith(b'MZ'))
            self.assertIn(b'RunProgram="portable-launcher.exe"', output.read_bytes())
            extract = root / 'unpacked'
            subprocess.run(['7z', 'x', '-y', '-o' + str(extract), str(output)], check=True, stdout=subprocess.DEVNULL)
            for name, content in files.items():
                self.assertEqual((extract / name).read_bytes(), content)
            self.assertTrue((extract / 'licenses/lzma-sdk.txt').is_file())

    def test_unified_release_collection_rejects_legacy_or_ambiguous_payload(self):
        workflow = yaml.safe_load((ROOT / '.github/workflows/experimental-unified-release.yml').read_text())
        script = next(step['run'] for step in workflow['jobs']['publish']['steps']
                      if step.get('name') == 'Collect successful builds from the same commit')
        for layout in ('portable', 'legacy', 'ambiguous'):
            with self.subTest(layout=layout), tempfile.TemporaryDirectory() as temp:
                root = Path(temp);mock = root / 'gh'
                mock.write_text('''#!/bin/bash
while (($#)); do
 if [[ "$1" == --dir ]]; then out="$2";break;fi
 shift
done
case "$out" in
 incoming/windows)
  if [[ "$LAYOUT" == legacy ]]; then touch "$out/qsstv.exe";
  else printf 'portable payload' > "$out/QSSTV-9.6.0-Windows-x86_64-portable.exe";fi
  if [[ "$LAYOUT" == ambiguous ]]; then touch "$out/second-portable.exe";fi;;
 incoming/deb) touch "$out/test.deb";;
 incoming/appimage) touch "$out/test.AppImage";;
esac
''')
                mock.chmod(0o755);output = root / 'env';output.touch()
                result = subprocess.run(['bash', '-e', '-o', 'pipefail', '-c', script], cwd=root,
                    env=dict(os.environ, PATH=str(root)+':'+os.environ['PATH'], LAYOUT=layout,
                             TARGET_SHA='abc12345abc', GH_REPO='example/test', GITHUB_ENV=str(output), win='1', deb='2', app='3'),
                    capture_output=True, text=True)
                if layout == 'portable':
                    self.assertEqual(result.returncode, 0, result.stderr)
                    self.assertEqual((root / 'release/QSSTV-Windows-x86_64-abc12345-portable.exe').read_bytes(), b'portable payload')
                    self.assertFalse(list((root / 'release').glob('*.zip')))
                else:
                    self.assertNotEqual(result.returncode, 0)
                    self.assertFalse(list((root / 'release').glob('*.exe')))

    def test_workflows_ship_only_portable_exe(self):
        for name in ('windows.yml', 'publish-prerelease.yml'):
            workflow = yaml.safe_load((ROOT / '.github/workflows' / name).read_text())
            steps = workflow['jobs']['windows']['steps']
            self.assertTrue(any('bash scripts/package-windows.sh' == step.get('run') for step in steps))
            upload = next(step for step in steps if step.get('uses', '').startswith('actions/upload-artifact'))
            self.assertEqual(upload['with']['path'], 'out-windows/*-portable.exe')
            self.assertTrue(any(step.get('run') == './tests/test_windows_portable.ps1' for step in steps))
        unified = (ROOT / '.github/workflows/experimental-unified-release.yml').read_text()
        self.assertNotIn('zip -qr', unified)
        self.assertIn('test "${#windows_exes[@]}" -eq 1', unified)


if __name__ == '__main__':
    unittest.main()
