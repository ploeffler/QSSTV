# Windows port: implementation and validation status

## Implemented in modernization PR #4
- Updated `src/qsstv.pro` to distinguish Windows and Unix-specific dependencies and source files.
- Added `src/sound/soundwindows.h` and `src/sound/soundwindows.cpp` as a Qt Multimedia-based Windows audio implementation.
- Updated sound configuration, application UI, transmit widget and platform-dependent rig/XML-RPC code to compile in the Windows build.
- Added `.github/workflows/windows.yml` for MSYS2/MinGW64, Qt 5 and a locally built Hamlib 4.6.3.
- Collected `qsstv.exe` and available runtime DLLs/Qt plugins into the `QSSTV-Windows-experimental` Actions artifact.

## CI evidence
The Windows workflow succeeded on `main` at merge commit `c7dd2dce`:
[Windows build #40361875979](https://github.com/ploeffler/QSSTV/actions/runs/40361875979).

This demonstrates a successful CI compile and package step. It does **not** demonstrate correct sound-card operation, CAT/PTT interoperability, camera capture, or successful SSTV/HAMDRM transmission and reception.

## Required manual acceptance tests
1. Extract the complete artifact and start `qsstv.exe` on a clean Windows machine.
2. Check sound device enumeration, input selection, output selection and sample-rate behavior.
3. Receive a known SSTV signal and compare the decoded image against a reference.
4. Transmit a known SSTV test image into a dummy load or controlled test setup and verify decoding.
5. Test HAMDRM RX/TX and file transfer with a compatible peer.
6. Check Hamlib CAT control, PTT transitions and device disconnect/reconnect behavior.
7. Check error handling and logs when audio or radio devices are unavailable.
8. Record OS version, radio, audio hardware, observed result and reproducible defects.

## Known limitations
- Windows runtime and hardware validation is still pending.
- The current build uses Qt 5 and qmake; migration to a newer toolchain is future work.
- Video capture and other platform-specific behavior require explicit manual validation.
- The existing Windows workflow's automatic experimental release step is restricted to pushes on `feature/windows-port-implementation`; builds on `main` upload Actions artifacts but do not publish a release.

See [PACKAGING.md](PACKAGING.md) and [RELEASE_NOTES_EXPERIMENTAL.md](RELEASE_NOTES_EXPERIMENTAL.md).
