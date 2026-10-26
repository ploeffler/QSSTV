# QSSTV packaging targets

## Linux

Produce a portable **AppImage** (`QSSTV-<version>-x86_64.AppImage`) from a reproducible Linux build. AppImage is the Linux single-file executable distribution format; `.app` bundles are normally associated with macOS.

Requirements:
- Build QSSTV with Qt and its runtime dependencies.
- Bundle Qt plugins, libraries, icons, desktop entry and executable using a maintained AppImage packaging tool.
- Verify bundled dependencies and runtime on a clean supported Linux distribution.
- Publish AppImage as a GitHub Actions artifact and attach it to tagged releases.
- Provide executable permission and usage instructions (`chmod +x ...AppImage`).
- Consider aarch64 AppImage after x86_64 is validated.

## Windows

Build and package a native Windows executable with required Qt runtime libraries; publish downloadable archives or installer.

## macOS (future)

A `.app` bundle is the native macOS format; not a substitute for Linux AppImage.

## Current blockers

The existing qmake project uses PulseAudio, ALSA and Video4Linux-specific sources and therefore needs platform-specific backend separation before Windows builds can succeed. No successful cross-platform build is claimed yet.
