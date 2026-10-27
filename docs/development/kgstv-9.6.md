# KG-STV preparation for QSSTV 9.6

Status: experimental native live RX/TX, standalone ASCII text and manual BSR are connected to the 9.6.0 application, alongside the offline WAV tool. The RX/TX widgets, 12/48 kHz PCM bridge, clock calibration, transmission ownership and cancellation are covered by device-free tests. Original KG-STV interoperability and real-device operation are unverified. See [native/live tests and remaining validation](../../tests/kgstv-tests.md).

User-facing announcement: [QSSTV 9.6 planned release notes](../../documentation/RELEASE_NOTES_9.6.md).

## Scope and delivery sequence

1. **Protocol and fixtures:** document framing, CRC, convolutional coding, whitening, packet types and image compression. Obtain known-image and text audio fixtures from the original application, with provenance and expected results.
2. **Offline receive:** implement MSK demodulation and packet decoding for recorded audio, then image reconstruction. Add 4L-FSK and optional error correction; declare exactly which combinations have been validated.
3. **Live receive:** connect the decoder to QSSTV audio, image display, callsign/status display and gallery. The third mode is now explicitly experimental; synthetic controller exchanges work end to end, but original-signal validation remains a release gate.
4. **Transmit:** implement compatible image/text packet generation and modulation, reuse the audio and CAT/PTT paths, and validate reception by the original application.
5. **BSR:** request missing blocks, respond with the requested image data, and validate completion, repeated requests, cancellation and RX/TX transitions.

The current native RX/TX/BSR implementation is experimental. It must not be described as verified compatible RX/TX support. No release date or implementation PR has been assigned.

## Text messaging is part of the planned feature

Native KG-STV support must include **sending and receiving standalone text messages**, including operation without a selected TX image. This is a separate user capability from callsign identification and image transfer.

Planned behavior:

- Provide a message input and an explicit **Send Text** action in KG-STV mode.
- Display received messages and local send status in a readable message history, with timestamps and sender callsign when available from the protocol/session.
- Preserve the original message content; keep received text as plain text.
- Confirm the original protocol's character encoding and encoded-byte length limit using reference messages. Enforce that limit before transmission and report unsupported characters; do not silently truncate or replace them. Do not assume UTF-8 interoperability.
- Support the validated modulation and coding combinations for text as well as images.
- Share the existing audio and CAT/PTT resources; prevent simultaneous image, text and BSR transmissions, and release PTT on completion, cancellation or error.
- Treat a successful local transmission as **sent**, without claiming remote delivery unless an actual protocol acknowledgement has been established.

The exact text packet structure and its relationship to callsign packets remain protocol research tasks. Both the live tabs and offline tool can generate/decode standalone ASCII text and separate identification packets. The live message input/history and exclusive TX ownership are implemented. Original text composition, non-ASCII encoding and length limits remain unverified; the current conservative send limit is 200 ASCII bytes.

## Protocol research baseline

The inspected reference is KG-STV 1.0.1 build 004, file version 1.0.1.3, SHA-256:

```
4e1d3d121c7259410364863d4a31d94e27be186c733e4a2005255c1ee963b116
```

Static analysis found a 63-bit synchronization sequence, a periodic 127-bit whitening sequence, a CRC polynomial x^16 + x^12 + x^5 + 1, and rate-1/2 convolutional coding with K=7. CRC initialization, bit ordering, framing and state transitions still require reference-signal validation as a complete chain.

The image-block header contains 38 information bits plus a 16-bit CRC before convolutional coding to 108 bits. Header fields include packet type, coding/modulation flags, coordinates, quality and payload bit length. Field interpretation varies by packet type.

Packet types currently identified:

| Type | Interpreted purpose |
| --- | --- |
| 0 | Text/log data; precise field composition remains to be checked |
| 1 | Image block |
| 2 | Retransmitted image block |
| 3 | End of an image pass |
| 4 | Request for one missing block, with coordinates in the header |
| 5 | Abort |
| 6 | Callsign/identification text |

Image compression uses six 8×8 coefficient blocks per 16×16 image block, quantization and Huffman coding. **Negative coefficient amplitudes differ from standard JPEG.** A standard JPEG decoder cannot consume the transmitted block payload unchanged. Numerical DC/DCT details and color reconstruction remain to be confirmed.

Local research material is stored in `analysis/kgstv/` beside this repository, including Ghidra projects, address-linked findings, extracted tables and the coefficient prototype. Those local files are not part of a release or a portable repository dependency. The older coefficient prototype passes five hand-constructed structural tests. The new native components additionally cover fixed packet vectors, image blocks, synthetic PCM and loss/repair exchanges. These are not an original-application interoperability result.

## Application integration points

| Area | Existing entry points | Integration / remaining validation |
| --- | --- | --- |
| Mode selection | `src/appglobal.h`, `src/mainwindow.cpp`, RX/TX widgets | Experimental third mode and settings integrated; retain original-signal/release gates |
| Receive pipeline | `src/mainwidgets/rxfunctions.cpp` | Native controller receives 12 kHz PCM with clock correction; validate physical devices and original signals |
| Transmit pipeline | `src/mainwidgets/txfunctions.h`, `src/mainwidgets/txfunctions.cpp` | Experimental preparation/transmission states and cancelable PCM output integrated |
| UI and images | `src/mainwidgets/rxwidget.cpp`, `src/mainwidgets/txwidget.cpp`, gallery | Partial images, block counts and plain-text history integrated; digital image archives use a KGSTV prefix |
| Audio and radio | `src/sound`, `src/rig` | Existing device selection and dispatcher/PTT ownership reused; real-device testing remains open |
| Build | `src/qsstv.pro` | Native components and controller compiled by qmake; Windows build validation remains open |

Preserve persisted mode values: the current SSTV/DRM values are 0 and 1. Audit all mode switches and two-way conditionals, especially code that treats every non-SSTV mode as DRM. A new enum entry alone is not sufficient integration. Keep KG-STV's BSR state distinct from HAMDRM's retransmission mechanism.

The UI now presents analog SSTV, digital HAMDRM and KG-STV (experimental) as separate choices. MSK/4L-FSK and coding options belong to the KG-STV settings. Do not expose a placeholder mode that silently falls through into an existing modem.

## Implementation provenance

Implement the interoperable protocol in native, portable source. Keep reference analysis separate from application code; do not ship the original EXE or copy decompiled application functions into QSSTV. Review reference-material and table provenance, licensing and redistribution terms before incorporating assets into the repository.

## Validation required for availability

- Record fixture origin, reference build, input image/text, modulation, coding option, sample rate and expected image/block results.
- Check framing and CRC with known vectors, including malformed and truncated packets and payload-length bounds.
- Validate image reconstruction, negative coefficients, block coordinates, missing blocks and reception beginning mid-transmission.
- Cover MSK and 4L-FSK, coded and uncoded payloads, noise, frequency offset and interrupted audio.
- Demonstrate original KG-STV -> QSSTV reception; before enabling TX, demonstrate QSSTV -> original KG-STV reception.
- Test standalone text messages in both directions without an image selected: short and maximum-length messages, supported non-ASCII characters, unsupported input, empty input and messages exceeding the encoded-byte limit.
- Check text history, sender association, repeated messages, corrupt/truncated packets, and text/image/BSR transitions including cancellation and PTT release.
- Check BSR requests/responses, stale state, duplicates, completion, abort and correct PTT release.
- Build supported Linux and Windows targets, then test audio devices and CAT/PTT on real hardware.
- Recheck existing analog SSTV and HAMDRM reception/transmission and saved settings.

Record evidence and known limitations in implementation PRs and the 9.6 release notes. The MMSSTV importer is integrated in the 9.6.0 development build. The same development build now includes experimental live KG-STV; original interoperability and physical radio validation remain pending. Historical release notes remain unchanged.

### External recording check — 2026-10-27

The user-supplied `KG-STV_4LFSK_Norm.mp3` (SHA-256 `6f65c15ba9065df2e0c781bbe9b3c19d890079cdc77985b96140e58b445613e7`, 99.5005 s, mono MP3) was converted to 12 kHz PCM and decoded offline in 4L-FSK mode. Identification TEST3 and 299/300 image blocks were recovered, with zero coefficient-block rejections, producing a recognizable BBC test card. One block remains missing; expected color fidelity and recording provenance are unverified. This is partial external receive evidence, not full original interoperability or outbound/text/BSR/hardware validation. Workspace evidence: `analysis/kgstv/reference-4lfsk/`.

Follow-up: Ghidra confirms the original BGR scanline convention. Native TX/RX color conversion now preserves that convention and uses the original inverse matrix constants, guarded by an independent chroma bit-vector test. Direct 24 kHz PCM conversion of this MP3 recovers **300/300** blocks; the live controller's 12 kHz input recovers **299/300**, missing (13,10). Linear upsampling did not resolve this and was not retained. Corrected colors look plausible; exact fidelity against original receiver output, outbound compatibility and radio hardware remain unverified.

Latest receive improvement: the 12 kHz 4L-FSK receiver now supplements the direct detector with a 16-tap Hann-windowed sinc half-sample path. The real controller recovers **300/300 blocks** from the same 12 kHz reference, recognizes the completed pass and produces a PNG byte-identical to the direct 24 kHz result. Direct decoding remains active to avoid FIR tail truncation. Duplicate hypotheses and both reset states are covered by native tests. This resolves the previously documented missing (13,10) block for this recording; wider channel and original interoperability testing remain pending.

External text receive check (2026-10-27): the separate 2.136236-second `KG-STV_4L-FSK_text.mp3` decodes through both the offline tool and production live controller at 12 kHz to exactly one unchanged `TEST3>The Quick Brown Fox Jumps Over The Lazy Dog 0123456789` message, with no separate identification or image. The reference runner now guards both recordings by SHA-256 and selects image/text assertions accordingly. This supports reception of this external ASCII sample; original outbound/text-encoding/length-limit/BSR and hardware validation remain pending. No runtime change was needed for this check.
