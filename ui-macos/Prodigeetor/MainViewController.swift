import AppKit

final class MainViewController: NSViewController {
  private let coreBridge = CoreBridge()
  private var editorView: EditorView?
  private var currentFilePath: String?

  override func loadView() {
    view = NSView()
    view.wantsLayer = true
    view.layer?.backgroundColor = NSColor.windowBackgroundColor.cgColor
  }

  override func viewDidLoad() {
    super.viewDidLoad()
    coreBridge.initializeCore()

    coreBridge.setText("""
    // Prodigeetor
    // Use Cmd+O to open a file, Cmd+S to save
    let greeting = "Hello, world!"
    print(greeting)
    """)

    // Create editor with initial size
    let initialSize = NSSize(width: 960, height: 2000)
    let editor = EditorView(frame: NSRect(origin: .zero, size: initialSize), coreBridge: coreBridge)
    editor.setFilePath("Sample.ts")
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

  func openFile() {
    let panel = NSOpenPanel()
    panel.canChooseFiles = true
    panel.canChooseDirectories = false
    panel.allowsMultipleSelection = false
    // Allow all file types - be more permissive
    panel.allowedContentTypes = []
    panel.allowsOtherFileTypes = true

    panel.begin { [weak self] response in
      guard let self = self, response == .OK, let url = panel.url else { return }

      do {
        let content = try String(contentsOf: url, encoding: .utf8)
        self.coreBridge.setText(content)
        self.currentFilePath = url.path
        self.editorView?.setFilePath(url.path)
        self.editorView?.scrollToTop()
        self.view.window?.title = "Prodigeetor - \(url.lastPathComponent)"
      } catch {
        let alert = NSAlert()
        alert.messageText = "Failed to open file"
        alert.informativeText = error.localizedDescription
        alert.alertStyle = .warning
        alert.runModal()
      }
    }
  }

  func saveFile() {
    if let filePath = currentFilePath {
      // Save to existing file
      saveToPath(filePath)
    } else {
      // Show save panel
      let panel = NSSavePanel()
      panel.canCreateDirectories = true
      panel.allowedContentTypes = [.text, .sourceCode, .plainText]

      panel.begin { [weak self] response in
        guard let self = self, response == .OK, let url = panel.url else { return }
        self.currentFilePath = url.path
        self.saveToPath(url.path)
        self.view.window?.title = "Prodigeetor - \(url.lastPathComponent)"
      }
    }
  }

  private func saveToPath(_ path: String) {
    do {
      let content = coreBridge.getText()
      try content.write(toFile: path, atomically: true, encoding: .utf8)
    } catch {
      let alert = NSAlert()
      alert.messageText = "Failed to save file"
      alert.informativeText = error.localizedDescription
      alert.alertStyle = .warning
      alert.runModal()
    }
  }
}
