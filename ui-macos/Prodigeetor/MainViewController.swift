import AppKit

final class MainViewController: NSViewController {
  private let coreBridge = CoreBridge()
  private var editorView: EditorView?

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
    let greeting = "Hello, world!"
    print(greeting)
    """)

    let editor = EditorView(frame: .zero, coreBridge: coreBridge)
    editor.translatesAutoresizingMaskIntoConstraints = false
    editor.setEditorFont("Menlo", size: 14)
    view.addSubview(editor)
    self.editorView = editor

    NSLayoutConstraint.activate([
      editor.leadingAnchor.constraint(equalTo: view.leadingAnchor),
      editor.trailingAnchor.constraint(equalTo: view.trailingAnchor),
      editor.topAnchor.constraint(equalTo: view.topAnchor),
      editor.bottomAnchor.constraint(equalTo: view.bottomAnchor)
    ])
  }
}
