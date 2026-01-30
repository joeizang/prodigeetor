import AppKit

/// Represents a single editor instance with its own buffer and view
final class EditorViewController: NSViewController {
  private let coreBridge = CoreBridge()
  private var editorView: EditorView?
  private(set) var currentFilePath: String?
  private(set) var displayName: String = "Untitled"
  private(set) var isDirty: Bool = false

  // Callback for when the tab should be renamed
  var onTitleChanged: ((String) -> Void)?
  var onDirtyStateChanged: ((Bool) -> Void)?

  override func loadView() {
    view = NSView()
    view.wantsLayer = true
    view.layer?.backgroundColor = NSColor.windowBackgroundColor.cgColor
  }

  override func viewDidLoad() {
    super.viewDidLoad()
    coreBridge.initializeCore()

    // Create editor with initial size
    let initialSize = NSSize(width: 800, height: 2000)
    let editor = EditorView(frame: NSRect(origin: .zero, size: initialSize), coreBridge: coreBridge)
    editor.setFilePath("untitled")
    editor.setThemePath("themes/default.json")
    editor.autoresizingMask = [.width]

    let scrollView = NSScrollView()
    scrollView.translatesAutoresizingMaskIntoConstraints = false
    scrollView.hasVerticalScroller = true
    scrollView.hasHorizontalScroller = false
    scrollView.documentView = editor
    scrollView.borderType = .noBorder
    scrollView.drawsBackground = true
    scrollView.backgroundColor = NSColor(red: 0.114, green: 0.129, blue: 0.161, alpha: 1.0)

    view.addSubview(scrollView)
    self.editorView = editor

    NSLayoutConstraint.activate([
      scrollView.leadingAnchor.constraint(equalTo: view.leadingAnchor),
      scrollView.trailingAnchor.constraint(equalTo: view.trailingAnchor),
      scrollView.topAnchor.constraint(equalTo: view.topAnchor),
      scrollView.bottomAnchor.constraint(equalTo: view.bottomAnchor)
    ])
  }

  // MARK: - File Operations

  /// Load content from a file path
  func loadFile(at path: String) throws {
    let content = try String(contentsOfFile: path, encoding: .utf8)
    coreBridge.setText(content)
    currentFilePath = path
    displayName = (path as NSString).lastPathComponent
    isDirty = false
    editorView?.setFilePath(path)
    editorView?.scrollToTop()
    onTitleChanged?(displayName)
    onDirtyStateChanged?(isDirty)
  }

  /// Load content with explicit text (for new files)
  func loadContent(_ content: String, displayName: String = "Untitled") {
    coreBridge.setText(content)
    self.displayName = displayName
    self.currentFilePath = nil
    self.isDirty = false
    editorView?.setFilePath(displayName)
    editorView?.scrollToTop()
    onTitleChanged?(displayName)
    onDirtyStateChanged?(isDirty)
  }

  /// Save current content to file
  func saveFile() throws {
    if let filePath = currentFilePath {
      try saveToPath(filePath)
    } else {
      throw EditorError.noFilePath
    }
  }

  /// Save to a specific path
  func saveToPath(_ path: String) throws {
    let content = coreBridge.getText()
    try content.write(toFile: path, atomically: true, encoding: .utf8)
    currentFilePath = path
    displayName = (path as NSString).lastPathComponent
    isDirty = false
    editorView?.setFilePath(path)
    onTitleChanged?(displayName)
    onDirtyStateChanged?(isDirty)
  }

  /// Show save panel and save
  func saveFileAs(completion: @escaping (Bool) -> Void) {
    let panel = NSSavePanel()
    panel.canCreateDirectories = true
    panel.allowedContentTypes = [.text, .sourceCode, .plainText]

    if let filePath = currentFilePath {
      panel.directoryURL = URL(fileURLWithPath: (filePath as NSString).deletingLastPathComponent)
      panel.nameFieldStringValue = (filePath as NSString).lastPathComponent
    }

    panel.begin { [weak self] response in
      guard let self = self, response == .OK, let url = panel.url else {
        completion(false)
        return
      }

      do {
        try self.saveToPath(url.path)
        completion(true)
      } catch {
        let alert = NSAlert()
        alert.messageText = "Failed to save file"
        alert.informativeText = error.localizedDescription
        alert.alertStyle = .warning
        alert.runModal()
        completion(false)
      }
    }
  }

  // MARK: - State

  func markDirty() {
    if !isDirty {
      isDirty = true
      onDirtyStateChanged?(isDirty)
    }
  }

  func getText() -> String {
    return coreBridge.getText()
  }

  func hasUnsavedChanges() -> Bool {
    return isDirty
  }
}

enum EditorError: Error {
  case noFilePath

  var localizedDescription: String {
    switch self {
    case .noFilePath:
      return "No file path specified"
    }
  }
}
