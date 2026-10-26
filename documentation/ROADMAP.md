# QSSTV development roadmap

## Project goals
1. Maintain reproducible Linux and Windows builds, with downloadable packages.
2. Modernize the Qt/qmake application without regressing SSTV or HAMDRM functionality.
3. Investigate KG-STV interoperability and implement it after protocol and licensing review.
4. Explore optional APRS status reporting with opt-in controls and rate limits.

## Completed in modernization PR #4
- [x] Added GitHub Actions CI for Windows, Linux AppImage and Linux Debian packages.
- [x] Added platform-specific Windows audio implementation based on Qt Multimedia.
- [x] Adjusted platform-dependent build configuration and code for Windows compilation.
- [x] Created experimental downloadable CI artifacts for all three package types.
- [x] Enabled Linux builds on `main`, the implementation branch, and pull requests targeting `main`.
- [x] Verified all three builds on merge commit `c7dd2dce`.

## Remaining milestones
- [ ] Perform hands-on RX/TX audio, device selection, CAT/PTT, SSTV and HAMDRM tests on Windows and Linux.
- [ ] Validate the AppImage and DEB on clean supported systems.
- [ ] Publish a clearly labeled experimental prerelease; do not call it stable until hardware testing is complete.
- [ ] Inventory existing SSTV/HAMDRM modes and add automated regression/smoke tests.
- [ ] Consider a supported Qt migration and CMake after compatibility analysis.
- [ ] Refactor DSP and platform-specific audio/rig interfaces further.
- [ ] Research KG-STV protocol/licensing and add interoperable tests before implementation.
- [ ] Design optional APRS status events, settings and transports; disable transmission by default.

## Source entry points
- `src/qsstv.pro`: qmake build configuration.
- `src/mainwindow.ui`: Qt user interface.
- `src/sstv`, `src/drmrx`, `src/drmtx`: modem implementations.
- `src/sound`, `src/rig`: audio and radio control.

All work should be reviewed through pull requests and preserve existing modes.
