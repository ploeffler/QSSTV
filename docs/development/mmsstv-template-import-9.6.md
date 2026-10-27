# MMSSTV template import for QSSTV 9.6

Status: integrated in the QSSTV 9.6.0 development build; official release pending. The Linux build and automated checks are recorded below; Windows and visual comparison with original MMSSTV rendering remain to be validated.

## Goal

Import existing MMSSTV `.mtm` templates into QSSTV's native `.templ` format so operators can reuse their layouts in the template editor and TX overlay selection. This is an application-wide feature, independent of KG-STV modem implementation. Imported overlays should also work with KG-STV images once that mode is available.

MMSSTV's [template workflow documentation](https://hamsoft.ca/pages/mmsstv/quick-start-graphics.php) identifies `.MTM` as its template file extension. The [original English manual](https://github.com/n5ac/mmsstv/blob/master/EMMSSTV.TXT) describes template objects and macros. These sources establish user behavior; they do not by themselves establish a complete binary format specification.

## User workflow

1. Choose **File → Import MMSSTV template...** in the QSSTV template editor and select an `.mtm` file.
2. Preview the converted layout, including its canvas size and any substitutions or unsupported items.
3. Keep supported elements editable and translate supported dynamic fields to QSSTV macros.
4. Save the result as a new native `.templ` template and make it available in the TX template selector.

Preserve the source file. Report conversion limitations before saving; do not silently drop objects, replace dynamic fields with stale values, or overwrite an existing native template.

## Format and conversion work

- Inspect representative `.mtm` fixtures and the original reader/writer, where available, before implementing a parser. Record format variants and fixture provenance.
- Identify canvas dimensions, object records, coordinates, layer order, text encoding, font/style/color information, transparency and embedded or referenced images.
- Map supported text, line, shape and image objects to native editor items while preserving placement and stacking.
- Establish an explicit macro mapping for station/contact callsigns, date/time, reports and other supported dynamic fields. Matching macro characters must not be assumed to have matching meanings.
- Handle missing fonts and external image resources with visible diagnostics. Resolve image paths relative to the imported template where applicable and ask the operator to locate missing resources.
- Treat unsupported object types, including any OLE objects, as conversion limitations. Do not activate embedded applications; offer a clearly marked static image fallback only when a usable preview exists.
- Keep import parsing bounded and reject truncated, malformed or unsupported files without leaving a partially modified editor scene.

## Existing integration points

- `src/editor/editor.cpp`: file actions and file dialogs; currently offers image and `.templ` files.
- `src/editor/editorscene.cpp`: native scene loading/saving using `QDataStream` and a magic number.
- `src/editor/graphicitems.*`, `src/editor/basegraphicitem.*`: editable scene objects.
- `src/mainwidgets/txwidget.cpp`: template selection and applying overlays.

Use a dedicated MMSSTV parser/converter that produces native scene objects. Adding `.mtm` to the existing file filter alone does not provide format support. Preserve native `.templ` compatibility.

## Validation and availability

Compare imported layouts with reference renders from MMSSTV: text and macros, fonts and substitutions, colors/transparency, image resources, layer order, and 320×240 / 320×256 layouts. Verify dynamic values at transmission time and behavior when fitting templates to other image sizes.

Test import -> native save -> reopen -> TX preview, malformed inputs, missing resources and unsupported records. Existing QSSTV templates must still load, edit and render correctly. Document supported object types and remaining restrictions in the [planned 9.6 release notes](../../documentation/RELEASE_NOTES_9.6.md) before announcing availability.

No complete format compatibility or release date is promised until representative original files have been tested.

## Implemented reader and compatibility limits

`src/editor/mmsstvtemplate.cpp` independently parses the little-endian stream described by the original `CDraw`, `CDrawGroup`, `CDrawText` and `CDrawPic` readers/writers in [MMSSTV Draw.cpp](https://github.com/n5ac/mmsstv/blob/master/Draw.cpp). It reads the one-byte VCL pen-style enum explicitly rather than assuming all fields are 32-bit integers. File sizes, object counts, recursion, strings, coordinates and bitmap dimensions are bounded. Parsing finishes before the editor scene changes.

Supported: group versions 0–2, line/rectangle/filled rectangle objects and native ellipses, text versions 0–7, picture versions 1–5 with embedded BMP, and color-bar versions 1–3 for solid/gradient/bitmap content. Groups are flattened in drawing order; individual objects remain editable. History-image placeholders map to QSSTV's last-received-image placeholder with a warning. Transparent picture color keys are converted to alpha. Gradients become embedded images.

Text preserves font, size, basic style, color and rotation; unavailable fonts produce a substitution warning. Horizontal/vertical four-color text gradients and simple contours map to native editable gradient/pen properties, with a warning about interpolation and edge differences. Embedded bitmap text fills are retained as tiled native text textures, with an alignment warning. Other shadow effects, zero decoration, alignment, perspective, stacked/vertical text, rounded frames are rasterized with a warning, bevel effects are simplified to solid outlines, and picture cropping is simplified with warnings. Layout fidelity must be checked in the preview. Embedded picture masks (rounded rectangles, ellipses and polygons) are baked into alpha transparency with a warning that mask vertices cannot be edited afterwards. History-image placeholders remain rectangular and report any unpreserved mask. OLE/custom objects, sound bars and unknown records are rejected explicitly; no such objects are executed or silently discarded.

Explicit macro mapping: `%m` (own callsign), `%c` (contact callsign), `%r` (report) and `%v` (version) retain their meanings; MMSSTV `%n` becomes QSSTV `%o` (contact operator), `%T` becomes `%t` (UTC time with colon), `%t` becomes `%T` (compact UTC time), `%D` becomes `%d` (UTC date), `%L` becomes `%D` (local date), `%U`/`%u` retain local time with/without colon, and `%C` retains the contact callsign in lowercase. Date formatting uses yyyy/MM/dd with a conversion warning. Unknown or semantically incompatible macros are escaped to visible literal text and reported, so `%q`, for example, cannot accidentally expand to QSSTV's own QTH. Font character sets select Windows-1252, Shift-JIS, GBK, Big5 or Windows-1251; other character sets use Windows-1252 with a warning.

The import preview uses native QSSTV objects. Cancel and parser failures preserve the editor contents. Importing into an unsaved canvas requires confirming replacement. Imported content is unsaved and has no source filename, so ordinary Save opens native `.templ` saving rather than writing QSSTV data into the `.mtm` source.

## Tests

`tests/test_mmsstv_import.cpp` exercises native scene conversion, save/reload and rendering; `tests/test_settings_path.cpp` covers the configuration path and migration independently. Test inputs constructed by the importer test cover primitive/text/image objects, nesting, macro semantics, every truncated prefix, unsupported objects and invalid counts.

Five original `def1.mtm` through `def5.mtm` examples from the reference repository are used as external compatibility fixtures, not redistributed here. Successful parsing and native round-tripping do not establish pixel equivalence with MMSSTV's own rendering.

### Validation recorded on 2026-10-27

- Full Linux x86_64 release build succeeded with Qt 5.15.18.
- Import integration test passed, including all five original reference templates and identical native rendering after save/reload.
- Configuration-path tests passed for the default HOME path, XDG override, legacy migration, destination precedence, backup preservation and migration failures.
- No original MMSSTV pixel comparison, Windows build or radio-hardware test has been performed for this change.

Run the integration checks using the [test instructions](../../tests/mmsstv-import-tests.md).

### Editor workflow and save reliability

Automated offscreen tests also drive the actual editor action and file/preview dialogs: cancel file selection, cancel preview, refuse replacement of an unsaved canvas, accept import, and save the resulting native template. The original `.mtm` bytes remain unchanged.

Template and PNG output use `QSaveFile` for atomic replacement. The editor only clears its modified state after a successful commit; failures display a warning and preserve unsaved changes. The selected output format is tracked separately from the scene type, so exporting an imported template as PNG cannot make the next Save write native template bytes under a `.png` filename. Tests cover this sequence and failed template/PNG saves.

### Editable text gradients and contours

MMSSTV text fill grades 1 and 2 use the four stored colors horizontally and vertically, respectively. They now map to QSSTV linear text gradients at 0/33/67/100 percent. Simple contour effect 1 uses the stored contour color and maps its outer expansion to a native vector pen. Both remain editable after `.templ` saving. MMSSTV's raster interpolation/dilation and Qt's gradient/vector rendering can differ, so the preview continues to report this conversion. More complex extrusion/embossing effects remain outside this mapping.

### Embedded picture masks

Embedded bitmap pictures now preserve rounded-rectangle, ellipse and polygon masks as alpha transparency. The image object can be moved, resized and saved natively; the imported mask is baked into its pixels, not stored as editable polygon vertices. Both tagged polygon records (count, coordinate dimensions, points) and the legacy count/points representation are supported, with normalization/edge-rendering warnings.

Polygon vertex counts, total vertices, coordinate dimensions and point ranges are bounded before rendering. Malformed/truncated polygon records fail before any editor content is replaced. Original MMSSTV transparent-overlay pictures retain their color-key behavior; MMSSTV ignores their shape masks. History-image placeholders cannot retain a dynamic mask in the native format and explicitly warn when converted to a rectangle.

Tests check opaque interior pixels, transparent exterior pixels, all three rounded-rectangle variants, ellipses, current and legacy polygons, malformed/truncated records, and pixel-identical native round-tripping. All five original MMSSTV examples and editor dialog tests still pass.

### Bitmap text fills

Text fill grade 3 now retains its embedded bitmap as a tiled text texture. The existing native template image field stores the tile, so no `.templ` format change is required. Text, macros and fonts remain editable. Applying a fill color or a gradient clears the text texture, allowing a normal solid/gradient fill again. Tiling origin can differ from MMSSTV's raster text bitmap and is reported in the preview.

Tests verify source tile colors, actual rendered red/blue texture pixels, native save/reload, replacement by solid/gradient fills, truncated bitmap rejection, and continued compatibility with the five reference templates and editor workflow tests.

### Time and callsign macro fidelity

The TX-template context now captures one instant for all date/time fields, preventing mixed values around midnight. UTC and local time are kept separate; local values use the operating system's time zone. Lowercase contact callsigns retain their MMSSTV meaning instead of falling through to literal text.

Tests use Europe/Vienna explicitly and verify summer time, winter time, a local date crossing midnight while UTC is still on the previous date, compact/colon time formatting, repeated context initialization and escaped literal macro strings. Macros without a compatible QSSTV value (such as MMSSTV's contact-QTH `%q`) continue to be preserved as literals with a warning.

### Rapport and contest suffix macros

MMSSTV `%R` and `%N` now map to native macros with the same identifiers. `%R` takes the first three characters of the TX RSV field; `%N` retains its exact suffix, including separators. For example, `595#007` produces `595` and `#007`; `595 012` produces `595` and ` 012`.

QSSTV's RSV input also accepts free text. These macros split the actual field; they do not validate or invent numeric reports. Unlike MMSSTV's historical fallback for a short `%R` field, an empty/short input is not replaced with `595`. The import preview warns about this deliberate difference. MMSSTV `%s` and `%M`, which use a separate received-report field that QSSTV does not provide here, remain escaped literals with warnings.

Fixed tests cover numbered reports, separators, empty/short reports and free text, and verify that unsupported received-report macros are not accidentally expanded as QSSTV SNR or another value.

### Shape and pen compatibility

Rectangle styles 4/5 now map to native editable ellipse objects, including filled ellipses. Rounded frame styles 1–3 become embedded images that preserve fill/stroke geometry; their curve is not separately editable. Filled objects preserve the MMSSTV fill/contour distinction.

The stored pen-style byte `255` represents the signed `-1` no-contour setting and is accepted rather than rejected. Styles 5/6 on MMSSTV lines and frames describe beveled effects, not an invisible Qt pen; they now remain visible as solid outlines with an explicit simplification warning. Invalid pen/style values still fail before replacing editor contents.

Tests cover native ellipse types, fill and transparent exterior pixels, filled no-contour objects, all three rounded-frame variants, bevel visibility, native save/reload and invalid style rejection. Existing reference templates and editor workflow tests remain part of the same suite.

### Whole-template sampled color key

MMSSTV samples the color at its saved transparency point after composing the template, then removes that color from the complete overlay. Applying transparency to each object separately is not equivalent when objects overlap. The importer now retains the root background color, sampling point and reference canvas. QSSTV samples/removes the color after native rendering and before compositing over the TX image, preserving editable text/objects.

The import preview shows the resulting transparency on a checkerboard. PNG export and TX overlay rendering use the same color-key path. The editor itself continues to show editable objects on the template background. Sampling occurs after the current text/image rendering, so font and raster differences from MMSSTV can affect a sampling point placed within an object; check the preview.

**Native compatibility:** templates using this feature are saved with a `9.6` format marker and an optional `MMCK` record containing background color, point and reference canvas. They require this new reader; older QSSTV builds will not load the new record. Older `.templ` files remain readable, and files without color-key metadata keep the legacy `9.0` format. Reimport the original `.mtm` to recover sampled-key settings missing from conversions saved by earlier test builds. This template-format marker does not rename the application configuration file.

Tests cover a sampling point inside an overlapping foreground object, correct cut-through to a separate TX background, PNG alpha, native persistence, legacy/new format markers, and invalid transparency points. The entire importer/editor suite and original MMSSTV examples are also checked.

### Editing transparency settings

The template editor now offers **Edit → Template transparency...** for imported and native templates. Enable/disable sampled transparency, choose the opaque template background and set the sample X/Y coordinates in current canvas pixels, or left-click the preview to pick a pixel. The live checkerboard preview marks the point with a crosshair. It is a preview-only marker, not template content.

The dialog previews a temporary native copy and commits settings only on OK. Cancel preserves content, selection and modified state; accepting unchanged settings does not mark a clean template dirty. Valid changes mark it modified. Coordinates/colors are validated before updating the live scene.

Disabling sampled transparency returns to ordinary native alpha rendering and omits the `MMCK` record on the next native save, restoring legacy serialization. Disabled sampling settings are not persisted in that legacy file. Tests drive the real dialog through edit/cancel/disable/no-op cases, check background/point persistence, reject invalid coordinates and verify return to the legacy format.

The preview converts mouse positions through the view transform into template pixel coordinates, updating X/Y together. Right-clicks, clicks outside the template and clicks while sampling is disabled leave the point unchanged. Numeric fields remain available for precise positioning. Automated tests send real Qt mouse events to the fitted preview and verify the selected/saved coordinates and ignored-click cases.

### Batch conversion

Choose **File → Batch import MMSSTV templates...**, select multiple `.mtm` files, then choose an existing destination directory. Each valid input becomes a native, editable `.templ` file; the open editor canvas is unchanged. Existing files, directories and symbolic links are skipped when choosing names (`name_2.templ`, `name_3.templ`, etc.). Each output is staged in the destination and moved into place without overwriting existing entries. Invalid inputs do not stop the remaining conversions. The final report lists output paths, conversion warnings and errors for each input. Source files remain unchanged. Choose the configured templates directory to make the outputs available to the template gallery.
