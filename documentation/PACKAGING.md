# QSSTV packaging and installation

## Current status
Modernization PR #4 was merged into `main` at commit `c7dd2dce`. The following GitHub Actions builds all completed successfully for that commit. This verifies compilation and packaging, **not** operation on real radio hardware.

| Platform | CI artifact | Verified workflow run |
| --- | --- | --- |
| Windows x86_64 | `QSSTV-Windows-experimental` | [40361875979](https://github.com/ploeffler/QSSTV/actions/runs/40361875979) |
| Linux x86_64 AppImage | `QSSTV-Linux-AppImage-experimental` | [40361876070](https://github.com/ploeffler/QSSTV/actions/runs/40361876070) |
| Linux amd64 Debian package | `QSSTV-Linux-DEB-experimental` | [40361875965](https://github.com/ploeffler/QSSTV/actions/runs/40361875965) |

Download artifacts from the **Artifacts** section of each run (GitHub sign-in may be required). Artifacts have retention limits. Extract the downloaded artifact ZIP before using its contents.

## Windows
The Windows workflow uses MSYS2/MinGW64 and Qt 5 and now produces **one portable file**, `QSSTV-<source-version>-Windows-x86_64-portable.exe` (currently `QSSTV-9.6.0-Windows-x86_64-portable.exe`).

1. Download the portable EXE from a release. An Actions artifact download is still a GitHub-generated ZIP wrapper: extract the single EXE from it once.
2. Double-click the EXE. It extracts its bundled application, DLLs and Qt plugins into a unique temporary directory and starts QSSTV without installation or administrator rights.
3. Exit QSSTV normally. The starter waits for it and removes the temporary runtime. It creates no Start Menu entry, desktop shortcut or uninstaller.
4. Test audio input/output, device selection and CAT/PTT before transmitting.

This is a self-extracting portable application, not a statically linked Qt executable. No separate Qt or MinGW installation is needed. Windows settings retain their existing per-user native storage; images and templates use the configured persistent directories. If `HOME` is absent, the launcher uses `USERPROFILE`; an explicit `HOME` is preserved. Settings are not embedded back into the EXE or automatically carried on a USB drive. An interrupted/killed starter or reboot can leave temporary files behind.

Shared packaging lives in `scripts/package-windows.sh`: deploy Qt including Windows audio/image plugins, recursively collect non-system PE imports (including Qwt, FFTW, OpenJPEG and plugin dependencies), and fail on unresolved DLLs. `qt.conf` resolves plugins inside the payload. The launcher is built with a static compiler runtime. `scripts/package-windows-portable.py` joins the hash-pinned official LZMA SDK 26.05 SFX, a config that runs the launcher, and a tested LZMA2 archive. Dependency notices and QSSTV's license are included internally.

Both Windows build workflows and both release collectors deliver the single portable EXE; collectors reject missing or ambiguous Windows artifacts. Published release assets are EXEs, not a second Windows ZIP. Existing published releases are not rewritten by this change.

The Windows jobs also create a device-free fixture EXE and test the actual SFX/launcher with a Windows-only PATH, Unicode/spaces, argument forwarding, HOME fallback/preservation, waiting and temporary-directory cleanup. This checks the starter, not QSSTV's actual audio/CAT/PTT or clean-machine GUI behavior. The new Windows CI run and manual hardware validation are pending; historical successful runs above used the former folder/ZIP packaging.

Local packaging checks (PyYAML, Bash and 7-Zip):

```bash
python3 tests/test_windows_packaging.py
python3 tests/test_unified_release.py
# Also assemble/extract a real SFX using the pinned official SDK archive:
QSSTV_LZMA_SDK=/path/to/lzma2605.7z python3 tests/test_windows_packaging.py
```

## Linux configuration location

Linux stores settings directly in `~/.config/qsstv_<application-version>.conf`, without an organization subdirectory. The current 9.6.0 build uses `~/.config/qsstv_9.6.0.conf`; `XDG_CONFIG_HOME` overrides the directory. The complete version comes from Qt's application version, initialized from `MAJORVERSION + MINORVERSION` at startup. `CONFIGVERSION` remains the independent legacy template/native-settings identity.

Before reading settings, the automatic upgrader checks the current-version file. If it exists, it takes precedence and is validated. Otherwise, the upgrader selects the numerically latest configuration no newer than the running application, from the direct directory and the historical organization subdirectory (`ON4QZ/`). At equal versions the direct file wins. This includes old `qsstv_9.0.conf` files and subsequent full-version files. Future-version configurations are ignored, so an older executable does not downgrade them. If there is no eligible source, a fresh empty versioned file is created.

Migration copies the complete source byte-for-byte, including unknown keys and Qt-encoded values. The original is retained for rollback, and an existing target is never overwritten or merged. This release requires no key/schema transformations. A startup lock serializes cooperating upgrade processes; a private temporary file and rename publish the result. Malformed, unreadable or unwritable selected files stop startup with an error rather than silently resetting settings. New files are readable/writable only by their owner. Windows and macOS retain their native settings storage and legacy identity.

The path/migration test uses only temporary directories and Qt Core:

```bash
g++ -std=c++11 -fPIC -Isrc/utils $(pkg-config --cflags Qt5Core) \
    tests/test_settings_path.cpp src/utils/qsstvsettings.cpp \
    $(pkg-config --libs Qt5Core) -o /tmp/qsstv-settings-test
/tmp/qsstv-settings-test
```

## Linux AppImage
1. Download and extract `QSSTV-Linux-AppImage-experimental`.
2. Make the extracted `.AppImage` executable: `chmod +x <filename>.AppImage`.
3. Launch the AppImage from a terminal to capture any diagnostics.

The AppImage is built on Ubuntu 22.04 and includes Qt bundling. Test on a clean Linux installation before claiming portability.

## Linux DEB
1. Download and extract `QSSTV-Linux-DEB-experimental`.
2. On a compatible Debian/Ubuntu amd64 system, install the extracted package with `sudo apt install ./qsstv_*.deb`.
3. Start QSSTV and check that required audio and radio interfaces work.

The CI-generated Debian version is `0.0.0+git<commit>`; it is not a stable semantic release number.

## CI and release policy
- Windows: `.github/workflows/windows.yml`.
- AppImage: `.github/workflows/linux-appimage.yml`.
- Debian: `.github/workflows/linux-deb.yml`.
- Debian packaging: `scripts/package-deb.sh`.
- All three build on `main` and on pull requests; Linux also builds on the implementation branch.
- Successful builds produce Actions artifacts, **not automatically a consolidated GitHub Release**.
- A test release must be labeled **pre-release**, list its source commit and disclose the lack of completed real-device testing.
- Never label a package stable until RX/TX, audio, SSTV/HAMDRM and CAT/PTT have been tested on supported platforms.

## macOS
macOS is not part of the current validated packaging matrix. A native `.app` bundle would be a separate future deliverable.

## 9.6.0 development integration

The source application now identifies itself as QSSTV 9.6.0 and includes MMSSTV single-template and batch import. Fedora package defaults and the manual prerelease workflow default are 9.6.0; the latter uses `RELEASE_NOTES_9.6.md`. This does not publish a release. Automatic snapshot DEBs retain their existing git-based version scheme. Linux configuration filenames follow the full application version, with automatic migration of older files. The separate template/native-settings identity remains 9.0.
