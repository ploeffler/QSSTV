# Running the MMSSTV importer integration test

First create a normal Qt 5 qmake build of QSSTV (see the project README). The test reuses that build's compiler configuration and production objects, replacing only `main.o` with its own test entry point. It needs the same libraries as QSSTV and uses Qt's offscreen platform plugin; it does not initialize the radio or audio subsystems.

```bash
python3 tests/run_mmsstv_import_test.py /path/to/qsstv-build
```

To additionally test the original MMSSTV example templates, obtain the [reference repository](https://github.com/n5ac/mmsstv), then run:

```bash
python3 tests/run_mmsstv_import_test.py /path/to/qsstv-build --reference /path/to/mmsstv
```

The optional reference directory must contain `def1.mtm` through `def5.mtm`. The templates are not redistributed in this repository. The fixtures used on 2026-10-27 had these SHA-256 hashes:

| File | SHA-256 |
| --- | --- |
| def1.mtm | `8143851282c5d22d820bef8d1971f61e7a9c04d97f782f15639227be2205de03` |
| def2.mtm | `353c0f1b854e75282e28765b3e6c8596b552e4b18cf987e21b001dfe69f8cabd` |
| def3.mtm | `8ff47dd21d2c7f5ad5f1c7ad156db162c4b6e0ed8ac2563bde05eb863915f1ab` |
| def4.mtm | `c85f3a316d020376dd330c7ae5bb9184533b1192e59cce324118ca8ebfe37f18` |
| def5.mtm | `d45b2a339286a1b8a62e5fe3b6efc3945f65708c8296df9ed4e07370b7729456` |

The test checks parser results, macros, embedded BMPs, nesting, all truncated prefixes of its generated fixture, unsupported objects and length bounds. It also converts into the production editor scene, saves native `.templ`, reloads and compares rendered pixels within QSSTV, excluding the editor's canvas border. This is a native round-trip check, not a comparison against MMSSTV's own Windows renderer.

Configuration tests are independent of the importer and use temporary HOME/XDG directories. Build/run instructions are in [PACKAGING.md](../documentation/PACKAGING.md#linux-configuration-location).

The test additionally opens the production editor and drives its real import action, file picker, preview, replacement confirmation and save dialogs using Qt timers. It checks cancellation at each stage, preservation of the `.mtm` source, failed-save modified state, PNG export followed by Save, and direct PNG write failures. Tests run exclusively in temporary settings/output directories. Setting `QSSTV_TEST_ARTIFACTS` to an existing directory optionally saves an import-preview screenshot there.

Text-style checks validate four source colors, horizontal/vertical direction, contour color/width, native editable properties and pixel-identical native save/reload for the converted styles.

Picture-mask checks cover three rounded-rectangle shapes, ellipse masks, tagged and legacy polygons, opaque/transparent sample pixels, malformed polygon counts and truncated point data, plus native alpha round-tripping. With `QSSTV_TEST_ARTIFACTS` set, the test also saves a grid of the imported mask shapes as `image-mask-preview.png`.

Bitmap-text tests cover embedded tile parsing, actual texture-colored glyph pixels, native persistence, switching back to a solid color or gradient, and rejecting truncated tile data.

Time/context tests map MMSSTV date/time/case macros through the production TX-template context with a fixed instant and an explicit Europe/Vienna time zone. They verify summer/winter offsets, midnight date differences, compact vs colon formatting, lowercase callsigns, literal percent escaping and replacing a previous context without duplicate expansions.

Rapport/contest tests use fixed expected strings for full reports, three-character prefixes and exact suffixes (including `#` and spaces), plus empty/short/free-text fields. They verify that unsupported received-report macros remain literal rather than expanding to SNR or fabricated reports.

Shape compatibility checks cover native filled/outline ellipses, the signed no-contour pen byte (255), rounded-frame rasterization, visible bevel fallbacks, invalid pen/rectangle styles and native round-tripping. Ellipse interior/exterior pixels are checked independently of the save/reload comparison.

Whole-template color-key tests deliberately place the sampling point inside an overlapping foreground object and verify cut-through to the TX background, rather than revealing a lower template object. They cover PNG alpha, metadata save/load, legacy/new format markers and invalid sample points in both MMSSTV and native metadata.

Transparency-editor tests drive the actual Edit-menu dialog, checking cancel/no-op behavior, changed-point dirty state, sampled output, custom background persistence, disabling back to legacy serialization, and rejection of invalid coordinates without modifying the live document. Optional artifacts include `transparency-editor.png`.

Transparency picking tests send press/release events through the actual preview viewport, including its fitted scene transform. Left-click selection updates both coordinates; right-click, outside-canvas and disabled-selection cases must preserve them.

Batch conversion tests cover mixed valid/missing inputs, continuation after failure, existing output preservation, repeated basename suffixes, unchanged source bytes, native output rendering and transparency metadata, missing destinations and temporary directory cleanup.

The real batch-dialog workflow is also exercised with Qt Widgets: multiple selection with spaces and Unicode filenames, canceling file or directory selection, one valid and one damaged input, collision suffixes, per-file warnings/errors and summary counts. It reloads the resulting native file, checks source/existing output bytes, and confirms that the open editor keeps its unsaved canvas, dirty flag and window title. The report screenshot is saved as `batch-import-report.png` when `QSSTV_TEST_ARTIFACTS` is set.

## MMSSTV 1.13a installer verification (2026-10-27)

The user-provided `_mmsstv_MMSSTV113A.exe` (SHA-256 `c03b3e863a3f1580672ddc9bc2b0740c911c508cb20d00b254eeb725c2084f4a`) was statically extracted without running its Windows code. Its readme identifies MMSSTV 1.13a FULL (September 29, 2010). All five bundled `DEF1.MTM` through `DEF5.MTM` files match the hashes above byte-for-byte. The production integration test passed with exact copies from this installer, including native save/reload and rendering equality within QSSTV. This confirms the supplied examples; custom templates remain subject to the documented unsupported-object, font and effect limitations. Original Windows rendering was not tested.
