import AppKit

/// Manages split panes, each containing a TabContainerViewController
final class SplitContainerViewController: NSSplitViewController {
  private var tabContainers: [TabContainerViewController] = []
  private var activeSplitIndex: Int = 0

  override func viewDidLoad() {
    super.viewDidLoad()

    // Configure split view
    splitView.dividerStyle = .thin
    splitView.isVertical = true

    // Create initial split pane
    addNewSplitPane()
  }

  // MARK: - Split Management

  func addNewSplitPane(orientation: NSUserInterfaceLayoutOrientation = .horizontal) {
    let tabContainer = TabContainerViewController()
    tabContainers.append(tabContainer)

    let splitItem = NSSplitViewItem(viewController: tabContainer)
    splitItem.canCollapse = false
    splitItem.holdingPriority = .defaultLow

    addSplitViewItem(splitItem)

    // Update orientation if needed
    splitView.isVertical = (orientation == .horizontal)

    // Focus the new split
    activeSplitIndex = tabContainers.count - 1
  }

  func removeSplitPane(at index: Int) {
    guard index >= 0 && index < tabContainers.count else { return }

    // Don't remove the last split pane
    guard tabContainers.count > 1 else { return }

    let tabContainer = tabContainers[index]
    let splitItem = splitViewItems[index]
    tabContainers.remove(at: index)
    removeSplitViewItem(splitItem)
    tabContainer.removeFromParent()

    // Update active split index
    if activeSplitIndex >= tabContainers.count {
      activeSplitIndex = tabContainers.count - 1
    }
  }

  func closeActiveSplitPane() {
    removeSplitPane(at: activeSplitIndex)
  }

  // MARK: - Split Navigation

  func focusNextSplitPane() {
    if activeSplitIndex + 1 < tabContainers.count {
      activeSplitIndex += 1
    }
  }

  func focusPreviousSplitPane() {
    if activeSplitIndex > 0 {
      activeSplitIndex -= 1
    }
  }

  func getActiveSplitPane() -> TabContainerViewController? {
    guard activeSplitIndex >= 0 && activeSplitIndex < tabContainers.count else { return nil }
    return tabContainers[activeSplitIndex]
  }

  // MARK: - Split Orientation

  func splitVertically() {
    splitView.isVertical = true
    addNewSplitPane(orientation: .horizontal)
  }

  func splitHorizontally() {
    splitView.isVertical = false
    addNewSplitPane(orientation: .vertical)
  }

  // MARK: - File Operations Forwarding

  func openFile() {
    getActiveSplitPane()?.openFile()
  }

  func saveActiveFile() {
    getActiveSplitPane()?.saveActiveFile()
  }

  func closeActiveTab() {
    getActiveSplitPane()?.closeActiveTab()
  }

  func createNewTab() {
    _ = getActiveSplitPane()?.createNewTab()
  }

  // MARK: - Tab Navigation Forwarding

  func selectNextTab() {
    getActiveSplitPane()?.selectNextTab()
  }

  func selectPreviousTab() {
    getActiveSplitPane()?.selectPreviousTab()
  }

  func selectTab(at index: Int) {
    getActiveSplitPane()?.selectTab(at: index)
  }
}
