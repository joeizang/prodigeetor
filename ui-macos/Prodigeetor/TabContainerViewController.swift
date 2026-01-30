import AppKit

/// Manages a collection of editor tabs
final class TabContainerViewController: NSViewController {
  private let tabBarView = TabBarView()
  private var contentContainer: NSView!
  private var editors: [UUID: EditorViewController] = [:]
  private var tabOrder: [UUID] = []
  private var activeTabId: UUID?

  override func loadView() {
    view = NSView()
    view.wantsLayer = true
    view.layer?.backgroundColor = NSColor.windowBackgroundColor.cgColor
  }

  override func viewDidLoad() {
    super.viewDidLoad()
    setupUI()
    setupCallbacks()

    // Create initial empty tab
    createNewTab()
  }

  private func setupUI() {
    // Tab bar at the top
    tabBarView.translatesAutoresizingMaskIntoConstraints = false
    view.addSubview(tabBarView)

    // Content container for editor views
    contentContainer = NSView()
    contentContainer.translatesAutoresizingMaskIntoConstraints = false
    view.addSubview(contentContainer)

    NSLayoutConstraint.activate([
      // Tab bar constraints
      tabBarView.topAnchor.constraint(equalTo: view.topAnchor),
      tabBarView.leadingAnchor.constraint(equalTo: view.leadingAnchor),
      tabBarView.trailingAnchor.constraint(equalTo: view.trailingAnchor),
      tabBarView.heightAnchor.constraint(equalToConstant: 35),

      // Content container constraints
      contentContainer.topAnchor.constraint(equalTo: tabBarView.bottomAnchor),
      contentContainer.leadingAnchor.constraint(equalTo: view.leadingAnchor),
      contentContainer.trailingAnchor.constraint(equalTo: view.trailingAnchor),
      contentContainer.bottomAnchor.constraint(equalTo: view.bottomAnchor)
    ])
  }

  private func setupCallbacks() {
    tabBarView.onTabSelected = { [weak self] tabId in
      self?.selectTab(tabId)
    }

    tabBarView.onTabClosed = { [weak self] tabId in
      self?.closeTab(tabId)
    }
  }

  // MARK: - Tab Management

  func createNewTab(withContent content: String? = nil, displayName: String = "Untitled") -> UUID {
    let tabId = UUID()
    let editorVC = EditorViewController()

    // Setup callbacks
    editorVC.onTitleChanged = { [weak self, tabId] title in
      self?.tabBarView.updateTab(id: tabId, title: title)
    }

    editorVC.onDirtyStateChanged = { [weak self, tabId] isDirty in
      self?.tabBarView.updateTab(id: tabId, isDirty: isDirty)
    }

    addChild(editorVC)
    editors[tabId] = editorVC
    tabOrder.append(tabId)

    // Add to tab bar
    tabBarView.addTab(id: tabId, title: displayName, isDirty: false)

    // Load content if provided
    if let content = content {
      editorVC.loadContent(content, displayName: displayName)
    }

    // Select the new tab
    selectTab(tabId)

    return tabId
  }

  func openFileInNewTab(path: String) throws {
    let tabId = UUID()
    let editorVC = EditorViewController()

    // Setup callbacks
    editorVC.onTitleChanged = { [weak self, tabId] title in
      self?.tabBarView.updateTab(id: tabId, title: title)
      self?.updateWindowTitle()
    }

    editorVC.onDirtyStateChanged = { [weak self, tabId] isDirty in
      self?.tabBarView.updateTab(id: tabId, isDirty: isDirty)
    }

    addChild(editorVC)
    editors[tabId] = editorVC
    tabOrder.append(tabId)

    // Load file
    try editorVC.loadFile(at: path)

    // Add to tab bar
    let fileName = (path as NSString).lastPathComponent
    tabBarView.addTab(id: tabId, title: fileName, isDirty: false)

    // Select the new tab
    selectTab(tabId)
  }

  func selectTab(_ tabId: UUID) {
    guard editors[tabId] != nil else { return }

    // Hide current editor
    if let currentId = activeTabId, let currentEditor = editors[currentId] {
      currentEditor.view.removeFromSuperview()
    }

    // Show new editor
    activeTabId = tabId
    tabBarView.setActiveTab(id: tabId)

    if let editorVC = editors[tabId] {
      editorVC.view.translatesAutoresizingMaskIntoConstraints = false
      contentContainer.addSubview(editorVC.view)

      NSLayoutConstraint.activate([
        editorVC.view.topAnchor.constraint(equalTo: contentContainer.topAnchor),
        editorVC.view.leadingAnchor.constraint(equalTo: contentContainer.leadingAnchor),
        editorVC.view.trailingAnchor.constraint(equalTo: contentContainer.trailingAnchor),
        editorVC.view.bottomAnchor.constraint(equalTo: contentContainer.bottomAnchor)
      ])

      updateWindowTitle()
    }
  }

  func closeTab(_ tabId: UUID) {
    guard let editorVC = editors[tabId] else { return }

    // Check for unsaved changes
    if editorVC.hasUnsavedChanges() {
      let alert = NSAlert()
      alert.messageText = "Do you want to save changes to \"\(editorVC.displayName)\"?"
      alert.informativeText = "Your changes will be lost if you don't save them."
      alert.addButton(withTitle: "Save")
      alert.addButton(withTitle: "Don't Save")
      alert.addButton(withTitle: "Cancel")
      alert.alertStyle = .warning

      let response = alert.runModal()
      switch response {
      case .alertFirstButtonReturn: // Save
        if editorVC.currentFilePath != nil {
          try? editorVC.saveFile()
        } else {
          // Show save panel
          editorVC.saveFileAs { saved in
            if saved {
              self.performCloseTab(tabId)
            }
          }
          return
        }
      case .alertSecondButtonReturn: // Don't Save
        break
      case .alertThirdButtonReturn: // Cancel
        return
      default:
        return
      }
    }

    performCloseTab(tabId)
  }

  private func performCloseTab(_ tabId: UUID) {
    guard let editorVC = editors[tabId] else { return }

    // Remove from view hierarchy
    if activeTabId == tabId {
      editorVC.view.removeFromSuperview()
    }

    // Remove from tab bar
    tabBarView.removeTab(id: tabId)

    // Remove from data structures
    editors.removeValue(forKey: tabId)
    tabOrder.removeAll { $0 == tabId }
    editorVC.removeFromParent()

    // Select another tab if this was active
    if activeTabId == tabId {
      activeTabId = nil
      if let nextTabId = tabOrder.last {
        selectTab(nextTabId)
      } else {
        // No tabs left, create a new one
        createNewTab()
      }
    }
  }

  // MARK: - File Operations

  func openFile() {
    let panel = NSOpenPanel()
    panel.canChooseFiles = true
    panel.canChooseDirectories = false
    panel.allowsMultipleSelection = false
    panel.allowedContentTypes = []
    panel.allowsOtherFileTypes = true

    panel.begin { [weak self] response in
      guard let self = self, response == .OK, let url = panel.url else { return }

      do {
        try self.openFileInNewTab(path: url.path)
      } catch {
        let alert = NSAlert()
        alert.messageText = "Failed to open file"
        alert.informativeText = error.localizedDescription
        alert.alertStyle = .warning
        alert.runModal()
      }
    }
  }

  func saveActiveFile() {
    guard let activeId = activeTabId, let editorVC = editors[activeId] else { return }

    if editorVC.currentFilePath != nil {
      do {
        try editorVC.saveFile()
      } catch {
        let alert = NSAlert()
        alert.messageText = "Failed to save file"
        alert.informativeText = error.localizedDescription
        alert.alertStyle = .warning
        alert.runModal()
      }
    } else {
      editorVC.saveFileAs { _ in }
    }
  }

  func closeActiveTab() {
    guard let activeId = activeTabId else { return }
    closeTab(activeId)
  }

  // MARK: - Navigation

  func selectNextTab() {
    guard let activeId = activeTabId,
          let currentIndex = tabOrder.firstIndex(of: activeId),
          currentIndex + 1 < tabOrder.count else { return }
    selectTab(tabOrder[currentIndex + 1])
  }

  func selectPreviousTab() {
    guard let activeId = activeTabId,
          let currentIndex = tabOrder.firstIndex(of: activeId),
          currentIndex > 0 else { return }
    selectTab(tabOrder[currentIndex - 1])
  }

  func selectTab(at index: Int) {
    guard index >= 0 && index < tabOrder.count else { return }
    selectTab(tabOrder[index])
  }

  // MARK: - Helpers

  private func updateWindowTitle() {
    guard let activeId = activeTabId, let editorVC = editors[activeId] else {
      view.window?.title = "Prodigeetor"
      return
    }

    let dirtyIndicator = editorVC.isDirty ? "● " : ""
    view.window?.title = "Prodigeetor - \(dirtyIndicator)\(editorVC.displayName)"
  }

  func getActiveEditor() -> EditorViewController? {
    guard let activeId = activeTabId else { return nil }
    return editors[activeId]
  }
}
