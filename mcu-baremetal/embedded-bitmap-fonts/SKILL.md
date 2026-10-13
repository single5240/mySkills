---
name: embedded-bitmap-fonts
description: "[Scope: global] Diagnose, generate, subset, and integrate bitmap fonts for embedded LCD/OLED/TFT UIs, especially Chinese text, blurry or merged strokes, baseline clipping, limited Flash, RGB565 strip rendering, and legacy ARMCC/Keil toolchains. Do not use for desktop typography or general image editing."
---

# Embedded Bitmap Fonts

Use this skill when an embedded display needs new text, Chinese localization, a
smaller font image, or font-rendering diagnosis. Preserve the existing display
driver, framebuffer model, and UI navigation unless the request explicitly
includes changing them.

## Establish the display contract first

Before editing, inspect the screen resolution, orientation, color format,
framebuffer or strip-buffer lifetime, glyph bit packing, current text encoding,
and compiler/toolchain. Extract the actual on-screen strings from source and UI
specification; do not compile prose from whole requirements documents into the
firmware font.

Check every intended string has a glyph before integration. Treat a missing
glyph as a build-time or review failure while developing; do not silently turn
it into blank space in a released UI.

## Choose and subset fonts

- Prefer an existing project font and existing glyph format. Keep separate
  sizes only when they have a clear role, such as 16 px body text, 24 px titles,
  and a fixed-width numeric readout.
- For small CJK text, prefer a native bitmap/BDF source when visual quality is
  critical. Read [references/native-bdf.md](references/native-bdf.md) when
  selecting or integrating BDF glyphs, diagnosing clipped baselines, or
  considering a bitmap size conversion.
- Treat a native bitmap size as intrinsic. Do not present a 16 px bitmap scaled
  by a non-integer factor as an equivalent native 24 px font: nearest-neighbor
  scaling makes stroke widths uneven, while interpolation followed by
  thresholding can merge or erase strokes. Prefer a native font at each target
  size, or keep the original size and preserve hierarchy through layout.
- If rasterizing a vector source is necessary, record source font, licence,
  pixel size, thresholding method, and the source file hash or version.
- Before tuning a monochrome threshold, generate several representative
  outputs and compare their bitmap bytes or hashes. Identical outputs mean the
  rasterizer is already producing binary coverage or the threshold is not on
  the effective path; stop tuning it and change the source or rasterization
  method instead.
- Keep the generator, generated C/H files, character manifest, and preview in
  sync. The generator must use repository-relative output paths and accept
  explicit source-font paths rather than relying solely on machine-local paths.
- Recalculate table offsets, bitmap size, glyph count, row stride, and the
  capacity of all index and offset types after subsetting.

## Integrate with the renderer

Honor the font's bit order, row stride, variable width, and baseline. Decode
UTF-8 defensively and use a sorted Unicode lookup table or another bounded,
deterministic lookup suitable for the target.

Distinguish true clipping from a low-looking baseline. Inspect each glyph's
ink bounds and source metrics, rasterize on a padded temporary canvas when
needed, then place it into the fixed cell with an explicit baseline. Reject or
report out-of-cell pixels instead of silently cropping them. Avoid solving
mixed glyph extents with one unexplained global vertical shift.

For an RGB565 partial-refresh renderer, write only pixels in the active strip
to its existing canvas. Clip glyph rows against the strip before iterating
pixels; do not allocate a full-screen 1-bit framebuffer merely to draw text.
Check title, list columns, status bar, footer hints, units, and highlighted
states for clipping and overlap at their real pixel coordinates.

When adding static menu pages, keep navigation and rendering separate from
settings, persistence, calibration, and communication logic unless those
features are explicitly requested.

## Legacy ARMCC / Keil source encoding

ARMCC5 can parse a UTF-8 source file using the active code page, even when
comments display correctly. If non-ASCII narrow string literals cause invalid
multibyte or missing-quote errors, encode their UTF-8 bytes as C escapes, for
example `"\\xE7\\x94\\xB5\\xE9\\x87\\x8F"` for `电量`.

Encode all bytes in such a literal, or split adjacent literals safely: `\\xNN`
escapes consume following hexadecimal characters. Keep human-readable Chinese
in comments, UI specifications, manifests, and generator input; the runtime
string must still be UTF-8 for the decoder.

## Keil and delivery checks

Add only the required font source files and include directory to the existing
target; do not switch compiler version, device pack, startup code, or display
driver. Build the target with its established Keil/ARMCC version and report
errors and warnings separately.

Before handoff, run `git diff --check`, inspect the diff and status, verify
glyph coverage, and report Flash/RAM changes when available. A successful build
does not replace target-hardware checks: verify Chinese strokes, inverse/highlight
states, strip boundaries, and the actual screen layout on the device. Review
previews at 1:1 pixels with representative dense glyphs and real UI strings;
zoomed previews are diagnostic aids, not substitutes for the target panel.
