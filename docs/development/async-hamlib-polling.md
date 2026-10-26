# Async Hamlib polling: implementation plan

Source: [ON4QZ/QSSTV PR #79](https://github.com/ON4QZ/QSSTV/pull/79).
Local documentation and release planning: [ploeffler/QSSTV PR #10](https://github.com/ploeffler/QSSTV/pull/10) (merged).

## Motivation
The current `src/mainwindow.cpp` timer calls `rigControllerPtr->getFrequency()` on the GUI thread. A slow CAT/Hamlib response can freeze the user interface.

## Proposed code changes (not yet applied)
- Move frequency polling to a single in-flight QtConcurrent worker and report completion with QFutureWatcher.
- Serialize Hamlib access, including init, disable, shutdown, frequency, mode, PTT and raw commands.
- Ensure shutdown and CAT reinitialization do not invalidate the rig handle while a worker is using it.
- Track consecutive polling failures and provide a safe, bounded reconnect/retry policy.
- Preserve the current display behavior for valid sub-1-MHz frequencies.
- Evaluate PTT latency explicitly: moving frequency polling alone does not prevent PTT waiting behind a blocked CAT operation.
- Include Qt Concurrent in the qmake project.

## Acceptance criteria
1. GUI remains responsive during slow and failed frequency reads.
2. No concurrent access to the same Hamlib rig handle; no use-after-free on Restart, Disable or Exit.
3. PTT on/off is not indefinitely blocked behind a poll; document tested worst-case latency.
4. CAT recovers after temporary disconnect without requiring a process restart.
5. XML-RPC mode continues working.
6. Tests cover clean polling, timeout, reconnect, CAT restart, exit during polling, and operation below 1 MHz.
7. Linux and Windows builds pass; verify on a real rig before release.

## Pull request tracking
| Change | Upstream PR | Local PR | State |
| --- | --- | --- | --- |
| Async rig frequency polling and lifecycle safety | [ON4QZ #79](https://github.com/ON4QZ/QSSTV/pull/79) | Pending implementation PR | Design/validation plan only |
| SSTV sync and VIS/FSK candidates | [ON4QZ #80](https://github.com/ON4QZ/QSSTV/pull/80) | [QSSTV #10](https://github.com/ploeffler/QSSTV/pull/10) | Planning merged; implementation pending |

No UI redesign, branding change or release version bump is included. Do not merge or publish without explicit approval.
