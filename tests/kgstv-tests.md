# Experimental KG-STV tests and offline signals

Status: native protocol, image-block, PCM and BSR components are integrated in the 9.6.0 source tree. The **KG-STV (experimental)** RX/TX tabs are connected to the existing audio workers and shared dispatcher/PTT path. Original KG-STV interoperability and physical audio/radio operation remain unverified. The current evidence supports an experimental development mode, not a compatible/stable release.

## Native component test

With Qt 5 Core/Gui development files available:

```sh
g++ -O2 -std=c++11 -fPIC -Isrc/kgstv $(pkg-config --cflags Qt5Core Qt5Gui) \
    tests/test_kgstv.cpp src/kgstv/kgstvprotocol.cpp src/kgstv/kgstvaudio.cpp \
    src/kgstv/kgstvimage.cpp src/kgstv/kgstvtransfer.cpp \
    $(pkg-config --libs Qt5Core Qt5Gui) -o /tmp/test-kgstv
/tmp/test-kgstv
```

Fixed vectors cover the augmented CRC (`123456789` -> `e5cc`), convolutional code output and a 108-bit encoded BSR header. They were computed from the statically recovered mathematical format independently of the production C++ implementation. Deterministic malformed entropy/header inputs are also exercised. AddressSanitizer and UndefinedBehaviorSanitizer passed locally; LeakSanitizer could not run in the sandbox/ptrace environment and was disabled for the successful sanitizer run. Additional tests cover CRC rejection, hard-decision Viterbi correction, all seven packet types, payload limits, repeated 4L headers/padding, every split of a control frame, stream polarity, resynchronization and bounded input.

Image tests use independent all-zero and negative-DC coefficient vectors, truncated/invalid entropy data, constant-color expectations, and a 320x240 generated image. They exercise native packet encode/decode, missing-block requests, type-2 repairs, duplicate handling, invalid coordinates, bounded request queues, cache replacement/generations and reset. After repair, the image equals the complete native-decoder baseline. This is not a pixel comparison against the Windows application.

Synthetic PCM tests exercise continuous-phase tones and packet acquisition at 12 and 48 kHz, timing offsets, arbitrary input chunks, consecutive packets and cancellation. No frequency offset, sound-card drift, original transmit filtering or noisy-channel performance has been validated. The low-level PCM objects are serialized by the live controller. That controller does not itself acquire radio resources.

## Offline fixture tool

Build separately, without starting QSSTV's devices:

```sh
mkdir -p /tmp/kgstv-offline-build
cd /tmp/kgstv-offline-build
qmake /path/to/QSSTV/tools/kgstv-offline/kgstv-offline.pro
make -j2
```

The tool reads/writes mono 16-bit PCM WAV files. Transmit output is 12000 Hz; the reader accepts rates from 12000 through 48000 that are integer multiples of 1200. Outputs are staged in the selected directory and never replace existing entries. Input images must be exactly 320x240 pixels. `--four-level` selects the experimental 4L-FSK path; MSK is the default. Quality indices run from 0 through 15, with larger indices giving stronger quantization; oversized blocks try stronger quantization to satisfy the wire bound. Payload FEC is enabled unless `--no-fec` is used.

```sh
kgstv-offline encode source.png image.wav --callsign OE6PLD
kgstv-offline decode image.wav received.png
kgstv-offline decode damaged.wav partial.png --bsr-out requests.wav
kgstv-offline repair source.png requests.wav repairs.wav
kgstv-offline decode damaged.wav repaired.png --append repairs.wav
kgstv-offline text message.txt message.wav --callsign OE6PLD
kgstv-offline decode-text message.wav received.txt
```

Image decode returns exit code 0 for all 300 blocks and 3 for a partial image. BSR generation requires a valid end-of-pass packet. Repair generation requires the original source image; use the same quality/modulation options as the first pass. This tool does not establish over-the-air sender/image association and cannot detect a request for a different source image. `--append` replays additional repair recordings into the same in-memory receive image; there is no persistent transfer-state format yet. A recording is assumed to belong to one image session. Interleaved senders, multiple image sessions and live RX/TX transitions remain integration work.

Standalone text is intentionally restricted to 1..200 ASCII bytes (plus optional separate identification), with printable ASCII, CR/LF and tab accepted. This conservative tool limit is not a claim about the original application's message limit or non-ASCII encoding. The bit-packet API itself handles arbitrary bytes. Non-ASCII received bytes are escaped for display; received content is not interpreted or executed.

Run both component and file-level tests with the built tool (also wired into the Linux DEB workflow):

```sh
python3 tests/run_kgstv_tests.py /tmp/kgstv-offline-build
```

It generates an independent BMP fixture, converts it to WAV, decodes it, erases a segment of audio, generates BSR requests and repairs, and compares the repaired PNG with the full native-decoder baseline. Standalone text, both modulation paths, output/source preservation, invalid text and malformed RIFF data are also covered. All waveforms originate from this implementation.

## Static reference provenance and open validation

The reference SHA-256 is `4e1d3d121c7259410364863d4a31d94e27be186c733e4a2005255c1ee963b116` (KG-STV 1.0.1 build 004). Protocol numerical tables are identified by source address in `src/kgstv/kgstvtables.h`; the implementation has no runtime dependency on the reference EXE and copies no decompiled functions. Research projects, disassembly and extraction metadata live separately in `analysis/kgstv/` beside this repository. Ghidra verification of the tone generation and sample-rate calculations is recorded in `analysis/kgstv/output/radio-verification.txt`.

Original transmission uses a 48000 Hz default device rate and a quarter-rate signal calculation. MSK uses 1200/1800 Hz, and 4L-FSK maps dibits 00/01/10/11 to 1200/1400/1600/1800 Hz. Both branches advance at 1200 symbols/s. The native fixture generator implements continuous-phase tones directly; it does **not** yet reproduce the original two-stage FIR filtering. Numerical DC/DCT/color rounding and complete packet/session sequencing require original-signal confirmation.

## Live controller and UI verification

`tests/test_kgstv_controller.cpp` is run by `run_kgstv_tests.py`. It connects independent controller peers through 48 kHz TX -> 12 kHz RX PCM, without devices. Cases cover standalone text, exclusive prepared/active jobs, image loss, BSR requests, canceled repair reservation restoration, full repair, remote abort, immutable caches, local send/cancel status and receive generations. Clock-calibration cases use 47952 and 48048 Hz on both modulation paths with 200-byte messages. A TX-only quality/FEC change during reception must not reset the demodulator. New controller code uses mutex-protected state; RX/TX worker stop flags and RX/TX/audio state flags use atomics.

`tests/test_kgstv_ui.cpp` instantiates the actual RX/TX widgets with production objects, without calling sound, CAT or XML-RPC initialization. It checks separate tabs, synchronized settings, RX auto-save preference, invalid text/empty BSR dialogs, native peer PCM reaching timestamped plain-text history and partial-image display, 320x240 TX composition with a zero compression-size setting, control locking, stale prepare/completion/progress events and stale image generations. Run it after a full qmake application build:

```sh
python3 tests/run_kgstv_ui_test.py /path/to/qsstv-build
python3 tests/test_kgstv_radio_paths.py
```

The latter compiles the actual production dispatcher-start and PCM-write method bodies with device-free doubles. It verifies that playback starts before PTT is asserted, playback failure leaves PTT released, canceled/blocked/timed-out output returns promptly, and stereo/other-channel PTT tone packing does not sign-extend negative samples. These are not driver/ioctl/CAT tests. The Linux DEB workflow includes both test paths; remote CI has not been executed in this local session.

The dispatcher retains device/PTT ownership. KG-STV uses bounded PCM writes, TX delay, optional VOX pre-tone, silence and drain handling. It performs no automatic BSR response, repeater transmission or FTP upload. The selected digital RX/TX image directories hold KG-STV PNGs; reception auto-save requires all 300 blocks, while the ordinary Save action allows partial images. Stop is an immediate local cancellation, not a promise that an abort packet reached the peer.

Required next: original known-image/text/BSR WAV fixtures; test both transfer directions and precise text encoding/limits; validate original filters, DC/DCT/color fidelity, noise, frequency/clock tracking and multiple/interleaved senders; then exercise actual device selection, CAT/PTT, cancellation and RX/TX transitions on supported hardware. Preserve existing SSTV/HAMDRM behavior throughout.

## Optional external 4L-FSK recording

`python3 tests/run_kgstv_reference_test.py OFFLINE_BUILD /path/to/KG-STV_4LFSK_Norm.mp3 /new/output.png`

Requires ffmpeg and the exact user-provided recording, SHA-256 `6f65c15ba9065df2e0c781bbe9b3c19d890079cdc77985b96140e58b445613e7`; no download or bundled recording. Tests the real controller with 12 kHz input: 300/300 blocks with parallel fractional timing, completed pass and corrected yellow/cyan BGR colors. The resulting PNG is byte-identical to separate offline decoding at 24 kHz. This reference does not validate TX, text, BSR, exact original rendering or physical devices.

The 12 kHz 4L-FSK receiver retains its direct detector and adds a bounded 16-tap Hann-windowed sinc interpolation path. Native tests check frame deduplication, final frames without artificial tails, arbitrary chunking and replay after reset with one-sample chunks. MSK and higher-rate paths are unchanged.

The same runner also accepts `KG-STV_4L-FSK_text.mp3` (SHA-256 `aa79e18f41f1172d88648f863e759c8fba530772f921a562fb314ace57f492f7`) with a new `.txt` output path. It checks exactly one message `TEST3>The Quick Brown Fox Jumps Over The Lazy Dog 0123456789` through the production controller, no fabricated separate sender and zero image blocks. This is external ASCII receive evidence; outbound original compatibility, non-ASCII, maximum lengths and BSR remain unverified. No recording is bundled or downloaded.
