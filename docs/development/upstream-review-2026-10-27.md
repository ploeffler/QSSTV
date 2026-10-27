# Upstream review and stability fixes — 2026-10-27

Repository: [ON4QZ/QSSTV](https://github.com/ON4QZ/QSSTV/pulls).
Compared with local `main` at `2a16053852c9d52e1434d333bf4d89ca29aabc5f`.
All nine open upstream PRs were inspected. This document records the reviewed
head SHAs so later upstream edits do not silently change the review scope.

Implementation branch: `fix/upstream-stability-batch`.
The five small fixes below are implemented on that branch; this does not mean
they are merged, hardware-validated, or included in an existing release.
No automatic merge or release is requested by this review.

## Adopted changes

| Upstream source / author | Reviewed head SHA | Local adaptation | Local commit |
| --- | --- | --- | --- |
| [#71 Serial PTT release](https://github.com/ON4QZ/QSSTV/pull/71), IvanGirderboot | `30efcd014fd00783ad66efafbcebaae782c63d80` | Use `mask`, not `~mask`, when setting inactive inverted DTR/RTS. Correct both release and initial idle configuration; upstream changes only release. Unix serial PTT path only. | `a511f62` |
| [#77 NET rigctl host:port](https://github.com/ON4QZ/QSSTV/pull/77), gaialabs | `46162c729648e24f74a5bc6040435dd07b2985eb` | Set `rig_pathname` through Hamlib configuration instead of writing the pathname array. Retain the supplied serial device or full network host:port string. | `391e777` |
| [#67 DRM header data pointer](https://github.com/ON4QZ/QSSTV/pull/67), dl1jbe | `659ef0c02068d76d82622875e413aa362407b677` | Reacquire `QByteArray::data()` at each parameter-block iteration, after `advance()` may have invalidated the previous pointer. Add an explanatory comment. | `953e593` |
| [#13 SSTV/DRM tabs during TX](https://github.com/ON4QZ/QSSTV/pull/13), Ruxton | `7a3d5a887bef92aeeb936da24f558e800ab2d5e5` | Include the settings tab widget in existing `enableButtons()` disabling/restoration. Preserve existing TX lifecycle. | `33f5fc2` |
| [#66 polling debug output](https://github.com/ON4QZ/QSSTV/pull/66), dl1jbe | `ed933c456ed859bb82a617f46c3f5fc7b740137c` | Remove the two unconditional `getFreq` / `got Freq` messages rather than leave commented-out statements. Other diagnostics remain. | `54d3215` |

Original implementation attribution is also recorded in each commit message.
These patches were adapted to the local tree; upstream branches were not merged wholesale.

## Reviewed changes reserved for separate work

| Source | Reviewed head SHA | Decision and remaining work |
| --- | --- | --- |
| [#79 background rig calls](https://github.com/ON4QZ/QSSTV/pull/79) | `8e98817c21941c1bfe733ccb6df861f01f7aa3e2` | Next larger stability task. Still not implemented locally. Follow [async Hamlib plan](async-hamlib-polling.md): serialize rig access, handle restart/exit and XML-RPC safely, bound timeout and PTT latency, test disconnection/recovery. The reviewed upstream head already incorporates changes following earlier review feedback; do not treat old review comments as proof those defects remain. |
| [#41 non-const Hamlib rig_caps](https://github.com/ON4QZ/QSSTV/pull/41) | `f441064f7e0394e6c64dc2a22ff6a6a8ea7aa4fe` | Evaluate against supported Hamlib versions first; current packaged builds succeeded. Do not import the unrelated `manual.doxy` to `manual.y` change. Ensure callback and comparator signatures both match the target API. |
| [#12 TX image alignment](https://github.com/ON4QZ/QSSTV/pull/12) | `7b4fef59b3345deb22e4313e0cc80aaf206be7dd` | Optional feature after stability fixes. Initialize the new `imageAlignment` field; test crop/fit/stretch, template composition, and controls during TX. Reviewed patch does not initialize that field. |
| [#80 UI refresh](https://github.com/ON4QZ/QSSTV/pull/80) | `e915114b7735e1f406bc7045e70ee02820448403` | Do not import the combined UI/rebranding patch. Evaluate only synchronization dropout tolerance and retrace threshold changes in isolated work, following [decoder plan](next-release-sstv-recovery.md). Higher dropout tolerance can delay loss detection; changing retrace from 290 to 350 ms needs recorded-signal regression testing. |

## Validation and limits

Completed locally:

- `python3 tests/test_serial_ptt.py`: compiles and executes the actual three
  modem-line update blocks extracted from production source, checking 108 cases
  across initialization, TX, release, line polarities, and starting line states.
  Checks preservation of unrelated bits as well as DTR/RTS values.
- `python3 tests/test_unified_release.py`: all four existing release readiness
  regression tests pass.
- `git diff --check` passes.

The PTT test is also run by the Linux DEB workflow. It tests bit operations,
not real `ioctl` calls, device drivers, serial errors, or RF transmission.
Local Qt and Hamlib development packages are absent, so full application
compilation is delegated to the existing Linux and Windows PR workflows.
Do not interpret a successful build as hardware or decoder validation.

Required before considering these fixes validated for a release:

1. Linux DEB, AppImage and Windows builds succeed for the PR head.
2. Unix serial PTT: verify positive and inverted DTR/RTS assertion, release,
   initial idle state, and preservation of unrelated modem lines on hardware.
3. Hamlib: verify NET rigctl on a non-default port and a normal serial CAT
   connection. Existing Hamlib error handling is retained; this is not a
   general CAT initialization/error-handling rewrite.
4. DRM: decode a known valid multi-parameter header and image on the supported
   Qt5 build; repeat on Qt6 when a supported Qt6 build is available. This patch
   does not claim comprehensive malformed-packet bounds checking.
5. TX: try switching SSTV/DRM tabs while preparing/transmitting; confirm controls
   are restored after normal completion, stop and failed preparation.
6. Confirm useful CAT error diagnostics remain with polling debug noise removed.

## Next implementation order

After CI and the focused manual checks, implement async CAT (#79) separately.
Keep decoder threshold experiments (#80), Hamlib API expansion (#41), and image
alignment (#12) in separate PRs with their own evidence and source attribution.
