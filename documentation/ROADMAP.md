# QSSTV development roadmap

## Project goals
1. Maintain reproducible Linux and Windows builds, with downloadable packages.
2. Modernize the Qt/qmake application without regressing SSTV or HAMDRM functionality.
3. Prepare native KG-STV support for the 9.6 series after protocol, provenance and interoperability validation.
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
- [ ] Deliver the [KG-STV milestones for the 9.6 series](../docs/development/kgstv-9.6.md): reference fixtures, offline/live RX, TX, and BSR, with compatibility validation at each stage.
- [ ] Design optional APRS status events, settings and transports; disable transmission by default.

## Planned for the 9.6 series: KG-STV

KG-STV is planned as a third mode alongside analog SSTV and digital HAMDRM. Experimental native protocol, image-block, synthetic PCM and BSR components and an offline WAV tool are implemented; an experimental live path is now connected to QSSTV audio, RX/TX tabs and the shared PTT lifecycle. The scope includes standalone text messaging with send and receive controls. Development starts with reception from recorded audio, followed by live reception, image/text transmission and BSR. Availability depends on interoperability and regression results; no release date is assigned.

- [x] Implement native packet, block-codec, synthetic PCM and BSR components with offline fixture tooling.
- [x] Integrate experimental live audio/UI, standalone text and manual BSR into the common TX/PTT lifecycle, with device-free controller/widget/radio-path tests.
- [ ] Establish original KG-STV image/text/BSR interoperability and validate real audio devices and CAT/PTT.

See the [planned 9.6 announcement](RELEASE_NOTES_9.6.md) and [implementation and validation plan](../docs/development/kgstv-9.6.md).

## Integrated in 9.6.0: MMSSTV template import

- [x] Implement [MMSSTV `.mtm` template import](../docs/development/mmsstv-template-import-9.6.md) with conversion preview, editable native objects, explicit macro mapping and native `.templ` export.
- [x] Integrate single-template and batch import into the 9.6.0 application and preserve configuration compatibility.
- [ ] Validate layout fidelity and disclose unsupported objects, font substitutions and missing resources before advertising availability.

## Source entry points
- `src/qsstv.pro`: qmake build configuration.
- `src/mainwindow.ui`: Qt user interface.
- `src/sstv`, `src/drmrx`, `src/drmtx`: modem implementations.
- `src/sound`, `src/rig`: audio and radio control.

All work should be reviewed through pull requests and preserve existing modes.

## Upstream review (2026-10-27)

See the [complete upstream review and change provenance](../docs/development/upstream-review-2026-10-27.md) for all nine open PRs, reviewed SHAs, the first five adapted fixes, validation limits, and deferred work. Implementation on a review branch does not imply release validation.
