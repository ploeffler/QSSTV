# QSSTV development roadmap

## Objectives
1. **Cross-platform builds:** maintain Linux builds and introduce Windows builds and downloadable artifacts using GitHub Actions.
2. **Code and UI modernization:** audit Qt 5/qmake dependencies, move toward a maintained Qt toolchain and CMake, separate DSP from platform-specific audio and UI, modernize Qt widgets, and add regression tests.
3. **KG-STV:** research the protocol and licensing, implement interoperable RX/TX components, and integrate them into the mode selector.
4. **APRS status messages:** optional, opt-in reporting of TX/RX events and image completion over APRS-IS or a local KISS TNC; configurable callsign/SSID, message content and rate limits.

## Milestones
- [ ] Inventory current dependencies, modules, and supported SSTV/HAMDRM modes.
- [ ] Establish reproducible Linux CI build and basic smoke tests.
- [ ] Build and package Windows version with a supported Qt toolchain.
- [ ] Refactor portable DSP, audio and CAT/PTT interfaces.
- [ ] Modernize the UI while preserving existing operating modes.
- [ ] Add KG-STV test vectors, receiver and transmitter.
- [ ] Add APRS event model, settings and transports, with transmission disabled by default.
- [ ] Publish versioned releases and installation documentation.

## Existing entry points
- `src/qsstv.pro`: qmake project.
- `src/mainwindow.ui`: main Qt UI.
- `src/sstv`, `src/drmrx`, `src/drmtx`: modem implementations.
- `src/sound`, `src/rig`: audio and radio interfaces.

Changes should be developed in branches and reviewed through pull requests. Do not break existing SSTV or HAMDRM functionality.
