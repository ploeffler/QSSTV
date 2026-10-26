# QSSTV 9.5.1 – Experimental cross-platform test build

**Status: PRE-RELEASE / EXPERIMENTAL — not a stable release.**

Source: the tagged commit built by the release workflow. This is the first combined experimental release of the Windows, Linux AppImage and Linux DEB packages.

## What's new
- Experimental Windows build using MSYS2/MinGW64, Qt 5 and Hamlib, with a Qt Multimedia audio backend.
- Linux x86_64 AppImage and Debian amd64 package builds.
- Automated GitHub Actions builds on `main` and pull requests.
- Platform-specific build configuration and compatibility fixes for Windows.
- English development roadmap, porting notes, packaging instructions and test checklist.

## Downloads
Download the Windows ZIP, Linux AppImage and Linux Debian package from the **Assets** section of this GitHub Release. All three packages are built from the same source revision.

## Important warning
CI confirms compilation and packaging only. Real-device testing for SSTV/HAMDRM receive/transmit, audio, CAT/PTT, dependency loading and system compatibility remains outstanding. Do not assume that this build is suitable for unattended or critical operation.

## Feedback checklist
Please include operating system and version, QSSTV build commit, hardware/audio interface, radio model, reproduction steps, expected behavior, actual behavior and relevant logs when reporting issues.

## Documentation
- [Packaging and installation](PACKAGING.md)
- [Windows port and validation](WINDOWS_PORT.md)
- [Development roadmap](ROADMAP.md)
