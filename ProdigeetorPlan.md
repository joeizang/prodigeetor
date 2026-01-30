# Prodigeetor Plan

## 1) Goals & Scope
- Fast, minimalist native editor for macOS + Linux
- Languages: C#, TypeScript/JavaScript, Swift, HTML, CSS, SQL (Postgres + SQLite first, MySQL later)
- Built-in terminal
- LSP-backed intelligence
- Extensions: minimal custom, TypeScript/JavaScript, strict permissions

### Non-goals (v1)
- Git UI
- Remote dev
- VS Code extension compatibility
- Windows support

## 2) Locked Decisions
- UI: Native AppKit on macOS, GTK4 on Linux
- Core: shared C++ engine
- Shells: thin native shells
- Keybindings: VS Code style
- Terminal on Linux: GTK VTE
- C# LSP: OmniSharp-Roslyn
- C# runtime: system-installed .NET (.NET 6+)
- Extensions: custom minimal TS/JS with strict permissions
- Packaging: .deb preferred (Linux), DMG for macOS
- Build: CMake for core + Linux UI, Xcode for macOS UI

## 3) High-Level Architecture
- C++ core shared across platforms
- macOS shell (Swift/AppKit) hosts editor view and terminal
- Linux shell (GTK4) hosts editor view and terminal
- LSP servers bundled and launched as child processes
- Extension host (Node) runs out-of-process with strict permissions

### Process Model
- UI shell <-> C++ core (in-process)
- C++ core <-> LSP servers (child processes, stdio JSON-RPC)
- C++ core <-> Node extension host (JSON-RPC over IPC)
- Terminal: core manages PTY, UI renders

## 4) Core Responsibilities (C++)
- Text engine (rope buffer)
- Cursor/selection + undo/redo
- Tree-sitter parsing (incremental)
- Syntax highlighting + symbol index
- LSP client and routing
- Workspace model + file watcher
- Settings and theme loading
- Diagnostics model
- Terminal PTY abstraction

## 5) macOS Shell (Swift/AppKit)
- Window, tabs, splits
- Text rendering using CoreText
- Terminal view
- Objective-C++ bridge to C++ core
- Keybinding layer (VS Code style)

## 6) Linux Shell (GTK4)
- Window, tabs, splits
- Text rendering with Pango/HarfBuzz
- Terminal via GTK VTE
- GObject/C++ bridge to C++ core
- Keybinding layer (VS Code style)

## 7) Language Support (Bundled)
### TypeScript/JavaScript
- LSP: tsserver
- Formatter: Prettier

### HTML/CSS
- LSP: vscode HTML/CSS language servers
- Formatter: Prettier

### C#
- LSP: OmniSharp
- Requires system-installed .NET

### Swift
- LSP: sourcekit-lsp
- Requires Swift toolchain

### SQL
- LSP: sqls
- Dialects: Postgres + SQLite (v1), MySQL (v2)

## 8) Tree-sitter Role
- Incremental parsing for fast, accurate syntax highlighting
- Structural data for symbol outline, folding, and selection expansion
- Complements LSP for semantic features

## 9) Extensions (Minimal Custom TS/JS)
### Runtime
- Node-based extension host (separate process)
- IPC: JSON-RPC
- Strict permission model (default deny)

### Manifest
- extension.json fields:
  - id, name, version, publisher
  - main (compiled JS entry)
  - activationEvents
  - contributes: commands, snippets, languages
  - permissions: fs_read, fs_write, network, shell

### Minimal API (v1)
- workspace: readFile, writeFile, listFiles, openFile
- editor: insertText, setSelection, getSelection, diagnostics
- commands: registerCommand, executeCommand
- window: showMessage, showQuickPick, showInput
- languages: registerFormatter, registerLintProvider

### Security
- Default deny for filesystem write, network, shell
- Permission prompts on install/first use
- Extension host crashes are isolated

## 10) Features (MVP)
- Open folder and file
- Tabs + split panes
- Find/replace
- Tree-sitter syntax highlighting
- LSP: go-to definition, hover, completion, diagnostics
- Outline/symbols
- Terminal pane
- Light/dark theme
- settings.json

## 11) Project Layout (Proposed)
- /core
- /ui-macos
- /ui-linux
- /extension-host
- /lsp-bundles
- /themes
- /settings
- /extensions/examples

## 12) Build & Packaging
- CMake for core + Linux UI
- Xcode for macOS UI
- Linux packaging: .deb (preferred), AppImage (optional)
- macOS: signed + notarized DMG

## 13) Telemetry & Diagnostics
- Off by default
- Local crash logs
- Optional submit dialog

## 14) Testing
- Unit tests for core buffer and LSP client
- Integration tests for LSP servers
- UI smoke tests

## 15) Milestones (12 Weeks)
### Weeks 1-2
- Core buffer + rendering pipeline
- AppKit and GTK shells show editor view
- File open/save

### Weeks 3-5
- Tree-sitter integration
- Tabs/splits + find/replace
- Terminal pane

### Weeks 6-8
- LSP client + TS/JS/HTML/CSS servers
- Diagnostics + hover + completion
- Settings + theming

### Weeks 9-10
- OmniSharp + Swift + SQL servers
- Formatter integration

### Weeks 11-12
- Packaging (.deb + DMG)
- Stability/performance pass
- Internal beta
