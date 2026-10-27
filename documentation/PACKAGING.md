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
The Windows workflow uses MSYS2/MinGW64 and Qt 5, compiles Hamlib and packages `qsstv.exe` with available Qt/runtime libraries.

1. Download and extract `QSSTV-Windows-experimental`.
2. Keep `qsstv.exe`, DLLs and plugin directories together.
3. Start `qsstv.exe` from the extracted directory.
4. Test audio input/output, device selection and CAT/PTT before transmitting.

The Windows build is **experimental**. Passing CI does not prove that all runtime dependencies or device drivers work on a user's computer.

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
