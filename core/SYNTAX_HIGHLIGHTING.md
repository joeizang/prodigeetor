# Syntax Highlighting Guide

This document describes how to use the Tree-sitter-based syntax highlighting in Prodigeetor.

## Overview

Prodigeetor uses [Tree-sitter](https://tree-sitter.github.io/tree-sitter/) for fast, incremental syntax highlighting. Tree-sitter provides accurate, real-time syntax highlighting that updates as you type.

## Supported Languages

The following languages are supported with Tree-sitter grammars:

- JavaScript (`.js`)
- TypeScript (`.ts`)
- TSX (`.tsx`)
- HTML (`.html`, `.htm`)
- CSS (`.css`)
- Swift (`.swift`)
- C# (`.cs`)
- SQL (`.sql`)

## Usage

### Basic Setup

```cpp
#include "core.h"
#include "syntax_highlighter.h"
#include "theme.h"

// Create core instance
prodigeetor::Core core;

// Get syntax highlighter
auto& highlighter = core.syntax_highlighter();

// Set the language
highlighter.set_language(prodigeetor::TreeSitterHighlighter::LanguageId::TypeScript);

// Load and set theme
prodigeetor::SyntaxTheme theme = prodigeetor::load_theme("themes/default.json");
highlighter.set_theme(theme);
```

### Highlighting Text

```cpp
// Get the text to highlight
std::string text = core.buffer().text();

// Perform syntax highlighting
std::vector<prodigeetor::RenderSpan> spans = highlighter.highlight(text);

// Use the spans for rendering
for (const auto& span : spans) {
    // span.range.start.line, span.range.start.column
    // span.range.end.line, span.range.end.column
    // span.style.foreground, span.style.background
    // span.style.bold, span.style.italic
}
```

### Language Detection

Detect language from file extension:

```cpp
prodigeetor::TreeSitterHighlighter::LanguageId detect_language(const std::string& filename) {
    if (filename.ends_with(".ts")) {
        return prodigeetor::TreeSitterHighlighter::LanguageId::TypeScript;
    } else if (filename.ends_with(".tsx")) {
        return prodigeetor::TreeSitterHighlighter::LanguageId::TSX;
    } else if (filename.ends_with(".js") || filename.ends_with(".jsx")) {
        return prodigeetor::TreeSitterHighlighter::LanguageId::JavaScript;
    } else if (filename.ends_with(".html") || filename.ends_with(".htm")) {
        return prodigeetor::TreeSitterHighlighter::LanguageId::HTML;
    } else if (filename.ends_with(".css")) {
        return prodigeetor::TreeSitterHighlighter::LanguageId::CSS;
    } else if (filename.ends_with(".swift")) {
        return prodigeetor::TreeSitterHighlighter::LanguageId::Swift;
    } else if (filename.ends_with(".cs")) {
        return prodigeetor::TreeSitterHighlighter::LanguageId::CSharp;
    } else if (filename.ends_with(".sql")) {
        return prodigeetor::TreeSitterHighlighter::LanguageId::SQL;
    }
    return prodigeetor::TreeSitterHighlighter::LanguageId::JavaScript; // default
}
```

## Render Spans

The `highlight()` method returns a vector of `RenderSpan` structures:

```cpp
struct RenderSpan {
    Range range;      // Start and end positions
    TextStyle style;  // Visual styling
};

struct Range {
    Position start;
    Position end;
};

struct Position {
    uint32_t line;
    uint32_t column;
};

struct TextStyle {
    Color foreground;
    Color background;
    bool bold;
    bool italic;
    bool underline;
};
```

## Integration with Rendering

### macOS (CoreText)

```swift
func renderSyntaxHighlighting(spans: [RenderSpan], in context: CGContext) {
    for span in spans {
        let range = NSRange(
            location: span.range.start.column,
            length: span.range.end.column - span.range.start.column
        )

        let color = NSColor(
            red: CGFloat(span.style.foreground.r) / 255.0,
            green: CGFloat(span.style.foreground.g) / 255.0,
            blue: CGFloat(span.style.foreground.b) / 255.0,
            alpha: CGFloat(span.style.foreground.a) / 255.0
        )

        // Apply color to text range
        // ... CoreText rendering code ...
    }
}
```

### Linux (Pango)

```cpp
void render_syntax_highlighting(const std::vector<RenderSpan>& spans, PangoLayout* layout) {
    PangoAttrList* attr_list = pango_attr_list_new();

    for (const auto& span : spans) {
        // Create color attribute
        PangoAttribute* fg_attr = pango_attr_foreground_new(
            span.style.foreground.r * 256,
            span.style.foreground.g * 256,
            span.style.foreground.b * 256
        );

        // Set range
        fg_attr->start_index = span.range.start.column;
        fg_attr->end_index = span.range.end.column;

        pango_attr_list_insert(attr_list, fg_attr);

        // Handle bold/italic
        if (span.style.bold) {
            auto* bold_attr = pango_attr_weight_new(PANGO_WEIGHT_BOLD);
            bold_attr->start_index = span.range.start.column;
            bold_attr->end_index = span.range.end.column;
            pango_attr_list_insert(attr_list, bold_attr);
        }
    }

    pango_layout_set_attributes(layout, attr_list);
    pango_attr_list_unref(attr_list);
}
```

## Themes

Themes map Tree-sitter capture names to visual styles. Example theme structure:

```json
{
  "name": "Default Dark",
  "type": "dark",
  "colors": {
    "keyword": "#C586C0",
    "function": "#DCDCAA",
    "string": "#CE9178",
    "comment": "#6A9955",
    "variable": "#9CDCFE",
    "type": "#4EC9B0",
    "number": "#B5CEA8",
    "operator": "#D4D4D4"
  }
}
```

### Tree-sitter Captures

Common capture names used by Tree-sitter grammars:

- `keyword` - Language keywords (if, for, class, etc.)
- `function` - Function names
- `function.call` - Function call sites
- `method` - Method names
- `method.call` - Method call sites
- `variable` - Variable names
- `variable.parameter` - Function parameters
- `property` - Object properties
- `type` - Type names
- `string` - String literals
- `number` - Numeric literals
- `boolean` - Boolean literals
- `comment` - Comments
- `operator` - Operators (+, -, *, etc.)
- `punctuation.bracket` - Brackets and braces
- `punctuation.delimiter` - Commas, semicolons
- `tag` - HTML/XML tags
- `attribute` - HTML/XML attributes

## Performance

Tree-sitter is designed for incremental parsing:

1. **Initial Parse**: Parses the entire document once
2. **Incremental Updates**: On text changes, only re-parses affected portions
3. **Fast Queries**: Highlighting queries are executed efficiently on the syntax tree

For best performance:
- Call `highlight()` only when the text changes
- Cache the result until the next change
- Consider highlighting only visible lines for very large files

## Incremental Highlighting (Future)

For very large files, implement incremental highlighting:

```cpp
// Only highlight visible lines
std::vector<RenderSpan> highlight_range(
    TreeSitterHighlighter& highlighter,
    const std::string& text,
    size_t start_line,
    size_t end_line
) {
    // Extract visible portion
    // Run highlighter on subset
    // Return spans for visible range
}
```

## Query Files

Tree-sitter grammars include `highlights.scm` query files that define which syntax nodes map to which captures. These are located in:

```
third_party/tree-sitter-javascript/queries/highlights.scm
third_party/tree-sitter-typescript/queries/highlights.scm
third_party/tree-sitter-html/queries/highlights.scm
third_party/tree-sitter-css/queries/highlights.scm
third_party/tree-sitter-swift/queries/highlights.scm
third_party/tree-sitter-c-sharp/queries/highlights.scm
third_party/tree-sitter-sql/queries/highlights.scm
```

## Notes

- Tree-sitter parsing is synchronous but very fast
- For optimal performance, highlight in a background thread for very large files
- The same Tree-sitter parse tree can be used for:
  - Syntax highlighting
  - Code folding
  - Symbol outline
  - Structural navigation (jump to next function, etc.)

## Complementary to LSP

Tree-sitter and LSP serve different purposes:

- **Tree-sitter**: Fast, local syntax analysis for highlighting, folding, outline
- **LSP**: Semantic analysis for completion, diagnostics, navigation across files

Use both together for the best experience!
