# Windows port: implementation audit

## Confirmed source-level blockers

- `src/qsstv.pro` unconditionally links `libpulse`, `libpulse-simple`, FFTW and Hamlib via pkg-config; its `!macx` sections include ALSA and Video4Linux on Windows as well. Replace `!macx` with explicit `unix:!macx` where Linux-only, and select libraries per platform.
- `src/sound/soundpulse.h` includes PulseAudio headers and `src/sound/soundalsa.h` includes ALSA headers. Neither is a native Windows backend.
- `src/sound/soundbase.h` already defines an abstract `soundBase` interface with `init`, `read`, `write`, flush and close methods. Reuse this interface for a Windows audio backend rather than changing modem DSP.
- `src/main.cpp` uses Qt application APIs; this entry point is not an immediate blocker.
- Audit references to `soundPulse`, `soundAlsa`, `getCardList`, `timespec`, `v4l2`, POSIX APIs and system-specific paths before making source changes.

## Proposed staged implementation

1. Refactor qmake platform conditions and dependency linking, preserving the working Linux build.
2. Add a Windows sound backend using Qt Multimedia or native WASAPI, matching the existing `soundBase` sample and buffer contract.
3. Make video capture optional initially, then replace Video4Linux with Qt Multimedia capture on Windows.
4. Resolve platform-dependent Hamlib, FFTW, OpenJPEG, Qwt and other library dependencies with a reproducible Windows toolchain.
5. Add a Windows CI compile job; then package Qt DLLs/plugins and application dependencies into a downloadable artifact.
6. Test RX/TX audio, device enumeration, CAT/PTT, SSTV and HAMDRM on real Windows hardware.

No Windows build or compatibility is claimed by this audit. Linux AppImage CI is tracked separately in PR #2.
