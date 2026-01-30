# LSP Integration Guide

This document describes how to use the LSP (Language Server Protocol) integration in Prodigeetor.

## Overview

The LSP integration provides language intelligence features like:
- Code completion
- Hover information
- Go to definition
- Find references
- Document symbols (outline)
- Real-time diagnostics (errors, warnings)

## Architecture

The LSP implementation consists of three main components:

1. **LSPClient** (`lsp_client.h/cpp`) - Handles communication with a single LSP server via JSON-RPC over stdio
2. **LSPManager** (`lsp_manager.h/cpp`) - Manages multiple language servers and routes requests
3. **LSP Types** (`lsp_types.h`) - Data structures for LSP protocol types

## Supported Languages (v1)

### TypeScript/JavaScript
- **Server**: `typescript-language-server`
- **Install**: `npm install -g typescript-language-server typescript`
- **Extensions**: `.ts`, `.tsx`, `.js`, `.jsx`
- **Language ID**: `typescript`

### HTML
- **Server**: `vscode-html-language-server`
- **Install**: `npm install -g vscode-langservers-extracted`
- **Extensions**: `.html`, `.htm`
- **Language ID**: `html`

### CSS
- **Server**: `vscode-css-language-server`
- **Install**: `npm install -g vscode-langservers-extracted`
- **Extensions**: `.css`, `.scss`, `.less`
- **Language ID**: `css`

## Usage from Core

### Initialization

```cpp
#include "core.h"

// Create core instance
prodigeetor::Core core;
core.initialize();

// Initialize LSP with workspace root
core.initialize_lsp("/path/to/workspace");
```

### Opening a File

```cpp
// Open a TypeScript file
core.open_file("file:///path/to/file.ts", "typescript");

// Open an HTML file
core.open_file("file:///path/to/index.html", "html");
```

### Document Updates

When the user edits the document, notify LSP:

```cpp
// After text changes
core.lsp_manager().didChange("file:///path/to/file.ts", updated_text);
```

### Code Completion

```cpp
core.lsp_manager().completion(
  "file:///path/to/file.ts",
  10, // line
  15, // character
  [](const std::vector<prodigeetor::lsp::CompletionItem>& items) {
    // Handle completion items
    for (const auto& item : items) {
      std::cout << item.label << std::endl;
    }
  }
);
```

### Hover Information

```cpp
core.lsp_manager().hover(
  "file:///path/to/file.ts",
  10, // line
  15, // character
  [](const std::optional<prodigeetor::lsp::Hover>& hover) {
    if (hover) {
      std::cout << hover->contents << std::endl;
    }
  }
);
```

### Go to Definition

```cpp
core.lsp_manager().gotoDefinition(
  "file:///path/to/file.ts",
  10, // line
  15, // character
  [](const std::vector<prodigeetor::lsp::LSPLocation>& locations) {
    for (const auto& loc : locations) {
      std::cout << "Definition at: " << loc.uri << std::endl;
    }
  }
);
```

### Document Symbols (Outline)

```cpp
core.lsp_manager().documentSymbols(
  "file:///path/to/file.ts",
  [](const std::vector<prodigeetor::lsp::DocumentSymbol>& symbols) {
    for (const auto& symbol : symbols) {
      std::cout << symbol.name << " (" << symbol.kind << ")" << std::endl;
    }
  }
);
```

### Diagnostics

Set up diagnostics callback during initialization:

```cpp
core.lsp_manager().onDiagnostics(
  [](const std::string& uri, const std::vector<prodigeetor::lsp::Diagnostic>& diagnostics) {
    for (const auto& diag : diagnostics) {
      std::cout << "[" << diag.severity << "] " << diag.message << std::endl;
    }
  }
);
```

### Processing Messages

Call this regularly (e.g., in the UI event loop) to process incoming LSP messages:

```cpp
// In your main loop
while (running) {
  // ... handle UI events ...

  core.tick(); // Process LSP messages

  // ... render ...
}
```

## Integration with UI Shells

### macOS (Swift/AppKit)

```swift
// In your EditorViewController
class EditorViewController: NSViewController {
    var core: Core! // Objective-C++ bridge to C++ core

    func viewDidLoad() {
        super.viewDidLoad()

        // Initialize LSP
        let workspaceURL = // ... get workspace URL
        core.initializeLSP(workspaceURL.path)

        // Setup timer to process LSP messages
        Timer.scheduledTimer(withTimeInterval: 0.016, repeats: true) { _ in
            self.core.tick()
        }
    }

    func fileDidOpen(url: URL, languageId: String) {
        core.openFile(url.absoluteString, languageId: languageId)
    }

    func textDidChange() {
        // Notify LSP of changes
        let text = core.buffer().text()
        core.lspManager().didChange(currentFileURI, text)
    }
}
```

### Linux (GTK4)

```cpp
// In your editor widget
void EditorWidget::on_realize() {
    Gtk::Widget::on_realize();

    // Initialize LSP
    m_core->initialize_lsp(m_workspace_path);

    // Setup idle callback to process LSP messages
    Glib::signal_idle().connect([this]() {
        m_core->tick();
        return true; // Keep calling
    });
}

void EditorWidget::on_file_opened(const std::string& uri, const std::string& language_id) {
    m_core->open_file(uri, language_id);
}

void EditorWidget::on_text_changed() {
    std::string text = m_core->buffer().text();
    m_core->lsp_manager().didChange(m_current_uri, text);
}
```

## Adding New Language Servers

To add support for a new language server:

```cpp
// In Core::initialize_lsp()
lsp::LanguageServerConfig config;
config.command = "your-language-server";
config.args = {"--stdio"};
config.extensions = {".ext1", ".ext2"};
config.languageId = "yourlanguage";
m_lsp_manager->registerLanguageServer("yourlanguage", config);
```

## Notes

- LSP servers communicate via stdin/stdout using JSON-RPC
- All LSP operations are asynchronous (use callbacks)
- The manager automatically routes requests to the correct server based on file extension
- Position coordinates are 0-based (line and character)
- URIs should use `file://` scheme

## Future Enhancements

- JSON library integration (currently using minimal string building)
- Incremental text synchronization
- Signature help
- Code actions
- Formatting support
- Workspace symbols
- Better error handling and recovery
