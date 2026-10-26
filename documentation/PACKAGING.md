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
