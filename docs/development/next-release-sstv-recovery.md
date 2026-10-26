# Next experimental release: SSTV receive robustness

Status: proposed; no decoder changes applied yet.

## Scope

Evaluate and, only after regression testing, selectively port the two **non-UI** changes from [ON4QZ/QSSTV PR #80](https://github.com/ON4QZ/QSSTV/pull/80):

1. **RX sync dropout recovery** — `src/sstv/syncprocessor.cpp`, sensitivity table.
   - Current rows (temporary / sustained out-of-sync limits): `10/15`, `10/20`, `15/30`, `15/30`.
   - Upstream proposal: `25/30`, `25/35`, `30/45`, `15/30`.
   - Do not blindly raise the fourth setting. Evaluate each sensitivity independently.
2. **Retrace threshold** — `src/sstv/visfskid.h`: `MINRETRACEWIDTH` from 290 ms to 350 ms.
   - Keep this separate from the sync recovery change so regressions can be attributed correctly.

## Explicit exclusions

Do **not** import UI styling, icons, splash screens, layouts, QSSTVMania branding, executable renaming, default data directories, Qt settings identity, README rewrite, or version bump from PR #80.

## Validation plan

- Establish baseline decoding results from the current main branch with reproducible WAV samples (record source, SSTV mode, sample rate, expected output).
- Test clean RX signals in representative wide and narrow modes.
- Inject short silences / dropout segments of increasing length mid-image; measure recovered image lines and time to regain synchronization.
- Test two consecutive images with a gap to catch delayed re-acquisition and false lock.
- Test noise-only and weak-signal inputs for false-positive sync.
- Separately test VIS and FSK ID recognition around retrace timing boundaries.
- Compare before/after decoded images, mode detection, logging, and memory/runtime behavior.
- Build and test supported Linux and Windows targets in CI where possible; hardware/audio-integration testing still requires manual validation.

## Release gate

- Implement the two proposals in **separate PRs**, after test fixtures are available.
- Require no regression on clean reception or mode recognition.
- Document limitations and test evidence in each PR.
- Keep release 9.5.8 unchanged; select the next experimental version only when fixes are validated.
- Never merge or publish without explicit approval.

## Change provenance and pull-request register

This register records both the **source PR** and the **local integration PR** for each change. Do not describe a proposed change as implemented before its implementation PR has been merged.

| Change | Source PR | QSSTV PR | Status | Notes |
| --- | --- | --- | --- | --- |
| RX synchronization dropout tolerance | [ON4QZ/QSSTV#80](https://github.com/ON4QZ/QSSTV/pull/80) | Not created yet | Proposed; not implemented | Proposed edits to `src/sstv/syncprocessor.cpp`; test independently |
| VIS/FSK retrace threshold | [ON4QZ/QSSTV#80](https://github.com/ON4QZ/QSSTV/pull/80) | Not created yet | Proposed; not implemented | Proposed 290 ms → 350 ms in `src/sstv/visfskid.h`; test independently |
| Preparation, scope and validation plan | [ON4QZ/QSSTV#80](https://github.com/ON4QZ/QSSTV/pull/80) | [ploeffler/QSSTV#10](https://github.com/ploeffler/QSSTV/pull/10) | Draft documentation PR; not merged | Documents candidates and release gates; does not change decoder behavior |
| UI redesign and QSSTVMania rebranding | [ON4QZ/QSSTV#80](https://github.com/ON4QZ/QSSTV/pull/80) | None | Explicitly rejected | No styling, SVG icons, layouts, splash screen or rebranding |

### Recording subsequent implementation work

When an implementation PR is created, update the corresponding row with its PR number and status. Each implementation PR must document:

- The original source PR and relevant files/lines or commits.
- The precise code changes adopted, altered or deliberately omitted.
- Baseline and modified decoding test results, including WAV fixture provenance.
- CI run links and any remaining manual radio/audio testing.
- Merge commit and experimental release tag **only after** those events actually occur.

### Release history distinction

[Experimental v9.5.8](https://github.com/ploeffler/QSSTV/releases/tag/v9.5.8) predates this work and does not include either proposed decoder modification. The next version number and release PR are not yet assigned.
