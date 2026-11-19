<div align="right">
  <strong>ENG</strong> &nbsp;|&nbsp; <a href="README.ru.md">RU</a>
</div>

<div align="center">
  <img src="Resources/Icon128.png" alt="SimpleRichText icon" width="96">
  <h1>SimpleRichText</h1>
  <p>Format individual pieces of Unreal Engine UI text without writing rich text tags by hand.</p>
  <p>
    <img src="https://img.shields.io/badge/Unreal%20Engine-5.7-0E1128?logo=unrealengine&amp;logoColor=white" alt="Developed with Unreal Engine 5.7">
    <img src="https://img.shields.io/badge/Blueprint-ready-2874A6" alt="Blueprint ready">
    <a href="LICENSE"><img src="https://img.shields.io/badge/License-MIT-green.svg" alt="MIT License"></a>
  </p>
</div>

SimpleRichText adds **Simple Rich Text Block**, a `URichTextBlock` subclass with an editor for separately formatted text segments. Edit each segment's text and formatting in the Details panel; the plugin generates the markup and updates the standard Text property. The widget also exposes functions for changing segments in Blueprint and C++.

## Features

- Set color, font, typeface, and size for each segment.
- Enter text as individual segments or in one field separated into pieces.
- Edit segments in the Details panel with a color picker, formatting menu, and insert/remove controls.
- Use standard Rich Text Block features such as style sets, decorators, and wrapping.
- Build and update text at runtime using Blueprint nodes or C++.
- Optionally regenerate the Text property from segments when the widget loads or rebuilds.

## Requirements

- Unreal Engine **5.7** was used for development and builds. Other UE 5 versions have not been tested and may require API adjustments.
- A C++ build environment configured for your Unreal Engine installation.

## Installation

1. Copy or clone this repository into your project's `Plugins/SimpleRichText` directory. The `SimpleRichText.uplugin` file should be directly inside that folder.
2. Regenerate your project files and build the project in your IDE or through Unreal Engine.
3. Open the project, enable **SimpleRichText** in **Edit ? Plugins** if needed, and restart the editor when prompted.


For C++ usage from another module, add `SimpleRichText` to that module's dependencies in its `.Build.cs` file and include `SimpleRichTextBlock.h`.

## Quick start

1. Open a Widget Blueprint and add **Simple Rich Text Block** from the **Simple** palette category.
2. Select the widget and add an entry to **Text Segments** in the **Content** section of Details.
3. Enter text and choose its color. Open the formatting menu to set a font, typeface, or size.
4. Use **+** to insert another segment after a row, or **?** to remove it. An inserted row copies the source row's formatting.
5. The generated markup appears in the standard **Text** property and the widget preview updates.

## Editing text

### Per Segment Fields

This is the default input mode. Each row has its own multiline text field and formatting controls. Editing segments generates markup in **Text**. In the editor, adding an empty segment does not overwrite existing hand-written text when the generated markup is empty.

### Single Field + Separator

Set **Input Mode** to **Single Field + Separator** and enter all text in **Combined Text**. The default separator is `|`:

```text
Hello |beautiful |world|!
```

The string is split into four pieces, including spaces. Piece *N* uses the format of segment *N*. The segment rows show a preview and formatting controls. Extra pieces copy the last existing format; removing pieces removes the trailing segments and their formats. Formats are matched by position, so inserting a separator in the middle shifts the formatting of subsequent pieces.

When switching to this mode, existing segments are joined if Combined Text is empty. Choose a separator that does not occur in your text: occurrences are treated as boundaries. An empty separator treats the whole string as one piece.

### Details properties

| Property | Purpose |
| --- | --- |
| **Text** | Standard Rich Text Block text; can also be edited manually. Segment updates replace it with generated markup. |
| **Text Segments** | Ordered list of text pieces and their formatting. |
| **Always Rebuild From Segments** | Off by default. When enabled, regenerates Text during load, synchronization, and widget rebuild, overwriting manual Text changes. |
| **Input Mode** | Selects per-segment fields or a single combined field. |
| **Combined Text** | Text split into pieces in single-field mode. |
| **Segment Separator** | Split string for single-field mode; defaults to `|`. |

Each `FSimpleRichTextSegment` contains `Text`, `Color` (white by default), an optional `Font` (`UFont` or `UFontFace`), an optional `Typeface` name, and `Size` (`0` uses the block's default size). An unset font or typeface uses the corresponding base style setting.

## Generated markup

Each nonempty segment becomes a `<seg>` run. For example:

```xml
<seg c="FF0000FF" f="/Game/Fonts/Roboto.Roboto" tf="Bold" s="32">Hello</>
```

| Attribute | Meaning |
| --- | --- |
| `c` | Color in sRGB `RRGGBBAA` hexadecimal form; always present. |
| `f` | Soft object path to a font asset; omitted when unset. |
| `tf` | Typeface name; omitted when unset. |
| `s` | Font size; omitted when zero or less. |

The plugin registers its decorator automatically. It starts from the block's default text style and applies the segment attributes. Segment text escapes `\`, `<`, and `>` before generating markup. The standard Text field can also contain your own rich text tags, but a subsequent segment update replaces the field with generated markup. Font assets are loaded from their soft paths when the decorator creates a run; ensure referenced assets are available in packaged builds.

## Blueprint and C++ API

Segment operations are exposed under the **SimpleRichText** Blueprint category. Indices are zero-based; invalid indices are ignored by setters/removal, and `GetSegmentText` returns empty text for an invalid index.

| Function | Purpose |
| --- | --- |
| `SetSegments` | Replace the segment array and refresh the widget. |
| `SetCombinedText` | Split combined text using the current separator, retaining formats by position. |
| `SetCombinedTextWithFormats` | Split text and apply supplied formats in order; the formats' own text fields are ignored. |
| `SetFormats` | Apply formats by position while preserving segment text; the last supplied format is reused for remaining segments. An empty array does nothing. |
| `SetSegmentFormat` | Replace one segment's formatting while preserving its text. |
| `SetSegmentText`, `SetSegmentColor`, `SetSegmentSize` | Change one segment. Size is clamped to at least zero. |
| `SetAllSegmentsColor` | Change every segment's color. |
| `AddSegment`, `InsertSegment`, `RemoveSegment`, `ClearSegments` | Add, insert, remove, or clear segments. `AddSegment` returns the new index. |
| `GetSegmentCount`, `GetSegmentText`, `GetPlainText`, `BuildMarkup` | Inspect the segments, their joined plain text, or the generated markup. |

`AddSegment` and `InsertSegment` use the supplied text and color. If the new segment has no font, it inherits the neighboring font and, if unset, typeface; a size of zero or less inherits the neighboring size. Insertion at index zero uses the first existing segment. `SetSegments` takes the supplied values directly.

For example, given a valid `USimpleRichTextBlock* RichText`:

```cpp
#include "SimpleRichTextBlock.h"

FSimpleRichTextSegment Greeting;
Greeting.Text = FText::FromString(TEXT("Hello, "));
Greeting.Color = FLinearColor::White;

FSimpleRichTextSegment Name;
Name.Text = FText::FromString(TEXT("world!"));
Name.Color = FLinearColor(1.0f, 0.3f, 0.4f);
Name.Size = 28;

RichText->SetSegments({Greeting, Name});
```

Segment mutations refresh the standard Text property. `ClearSegments()` therefore clears generated text; keep separately authored markup elsewhere if you need to restore it.


## License

Licensed under the [MIT License](LICENSE). Copyright @ 2026 AlisaNyaXD. See the license file for the full terms.
