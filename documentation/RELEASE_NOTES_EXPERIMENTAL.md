# QSSTV experimental cross-platform test build

**Status: PRE-RELEASE / EXPERIMENTAL — not a stable release.**

Source baseline: `main`, modernization merge commit `c7dd2dce71391829841772f951beebf1f8e6da9b` (26 October 2026).

## What's new
- Experimental Windows build using MSYS2/MinGW64, Qt 5 and Hamlib, with a Qt Multimedia audio backend.
- Linux x86_64 AppImage and Debian amd64 package builds.
- Automated GitHub Actions builds on `main` and pull requests.
- Platform-specific build configuration and compatibility fixes for Windows.
- English development roadmap, porting notes, packaging instructions and test checklist.

## Downloads
The three successful CI artifacts for the baseline commit are available from:
- [Windows](https://github.com/ploeffler/QSSTV/actions/runs/40361875979)
- [Linux AppImage](https://github.com/ploeffler/QSSTV/actions/runs/40361876070)
- [Linux DEB](https://github.com/ploeffler/QSSTV/actions/runs/40361875965)

Open the workflow run's **Artifacts** section and extract the downloaded ZIP. These are temporary CI artifacts, not yet assets attached to a GitHub Release.

## Important warning
CI confirms compilation and packaging only. Real-device testing for SSTV/HAMDRM receive/transmit, audio, CAT/PTT, dependency loading and system compatibility remains outstanding. Do not assume that this build is suitable for unattended or critical operation.

## Feedback checklist
Please include operating system and version, QSSTV build commit, hardware/audio interface, radio model, reproduction steps, expected behavior, actual behavior and relevant logs when reporting issues.

## Documentation
- [Packaging and installation](PACKAGING.md)
- [Windows port and validation](WINDOWS_PORT.md)
- [Development roadmap](ROADMAP.md)
