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
    editor.setFilePath("Sample.ts")
    editor.setThemePath("themes/default.json")

    let scrollView = NSScrollView()
    scrollView.translatesAutoresizingMaskIntoConstraints = false
    scrollView.hasVerticalScroller = true
    scrollView.hasHorizontalScroller = false
    scrollView.documentView = editor
    scrollView.borderType = .noBorder

    view.addSubview(scrollView)
    self.editorView = editor

    NSLayoutConstraint.activate([
      scrollView.leadingAnchor.constraint(equalTo: view.leadingAnchor),
      scrollView.trailingAnchor.constraint(equalTo: view.trailingAnchor),
      scrollView.topAnchor.constraint(equalTo: view.topAnchor),
      scrollView.bottomAnchor.constraint(equalTo: view.bottomAnchor)
    ])
  }
}
