# Component API Specification (Draft)

This document defines the interfaces between:
1) UI shells (AppKit/GTK) and the C++ core
2) C++ core and the Node-based extension host

All APIs are versioned and should be backward-compatible within major releases.

## 1) Core <-> UI Shell API

### 1.1 Core Initialization
- `CoreHandle core_create()`
- `void core_destroy(CoreHandle)`
- `void core_initialize(CoreHandle, const CoreInitOptions &options)`

`CoreInitOptions`
- `config_path: string`
- `theme_path: string`
- `locale: string`
- `keybindings_path: string`

### 1.2 Workspace & Files
- `WorkspaceId core_open_workspace(path: string)`
- `void core_close_workspace(WorkspaceId)`
- `DocumentId core_open_document(path: string)`
- `void core_close_document(DocumentId)`
- `bool core_save_document(DocumentId)`
- `bool core_save_document_as(DocumentId, path: string)`

### 1.3 Editor Operations
- `void core_insert_text(DocumentId, Position, text: string)`
- `void core_delete_range(DocumentId, Range)`
- `void core_set_selection(DocumentId, Selection)`
- `Selection core_get_selection(DocumentId)`
- `void core_undo(DocumentId)`
- `void core_redo(DocumentId)`

### 1.4 Navigation & Symbols
- `Location core_go_to_definition(DocumentId, Position)`
- `Location[] core_references(DocumentId, Position)`
- `Symbol[] core_document_symbols(DocumentId)`
- `Hover core_hover(DocumentId, Position)`

### 1.5 Diagnostics
- `Diagnostic[] core_get_diagnostics(DocumentId)`

### 1.6 Events (Core -> UI)
UI shells subscribe to core events and update views accordingly.

Events:
- `on_document_changed(DocumentId, ChangeSet)`
- `on_diagnostics_changed(DocumentId, Diagnostic[])`
- `on_symbols_changed(DocumentId, Symbol[])`
- `on_theme_changed(Theme)`
- `on_keybindings_changed(Keymap)`
- `on_status_update(StatusMessage)`

### 1.7 Types
- `Position { line: u32, character: u32 }`
- `Range { start: Position, end: Position }`
- `Selection { anchor: Position, active: Position }`
- `Location { uri: string, range: Range }`
- `Diagnostic { range: Range, severity: enum, message: string, source: string }`
- `Symbol { name: string, kind: enum, range: Range, selectionRange: Range }`

## 2) Core <-> Extension Host API (JSON-RPC)

### 2.1 Transport
- IPC via stdio or local domain socket
- JSON-RPC 2.0
- All messages include `protocolVersion`

### 2.2 Extension Host Lifecycle
- `core.initializeExtensionHost(options)`
- `core.registerExtension(manifest)`
- `core.activateExtension(extensionId, reason)`
- `core.deactivateExtension(extensionId)`

### 2.3 Extension Permissions
- `core.requestPermissions(extensionId, permissions[]) -> { granted: bool, denied: string[] }`

### 2.4 Commands
- `core.registerCommand(extensionId, commandId, title)`
- `core.executeCommand(commandId, args[])`
- `core.onCommandInvoked(commandId, args[])` (event from core to host)

### 2.5 Workspace
- `core.workspace.readFile(uri) -> { content, encoding }`
- `core.workspace.writeFile(uri, content, encoding)`
- `core.workspace.listFiles(rootUri, glob)`
- `core.workspace.openFile(uri)`

### 2.6 Editor
- `core.editor.insertText(docId, position, text)`
- `core.editor.getSelection(docId)`
- `core.editor.setSelection(docId, selection)`
- `core.editor.showDiagnostics(docId, diagnostics[])`

### 2.7 Window/UI
- `core.window.showMessage(type, message)`
- `core.window.showQuickPick(items[]) -> selection`
- `core.window.showInput(prompt) -> value`

### 2.8 Languages
- `core.languages.registerFormatter(extensionId, languageId, formatProvider)`
- `core.languages.registerLintProvider(extensionId, languageId, lintProvider)`

### 2.9 Events (Core -> Extension Host)
- `onDidOpenDocument(doc)`
- `onDidChangeDocument(doc, changes)`
- `onDidSaveDocument(doc)`
- `onDidCloseDocument(doc)`
- `onDidChangeWorkspace(workspace)`

## 3) Versioning
- `protocolVersion: 1` in all IPC messages
- Additive changes within major releases
- Deprecations require one major release window
