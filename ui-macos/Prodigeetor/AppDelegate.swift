import AppKit

final class AppDelegate: NSObject, NSApplicationDelegate {
  private var window: NSWindow?
  private var splitContainerViewController: SplitContainerViewController?

  func applicationDidFinishLaunching(_ notification: Notification) {
    NSApp.setActivationPolicy(.regular)
    NSApp.activate(ignoringOtherApps: true)

    let viewController = SplitContainerViewController()
    self.splitContainerViewController = viewController

    let window = NSWindow(contentViewController: viewController)
    window.styleMask.insert([.titled, .closable, .miniaturizable, .resizable])
    window.isReleasedWhenClosed = false
    window.backgroundColor = NSColor.windowBackgroundColor

    window.setContentSize(NSSize(width: 1200, height: 700))
    window.center()
    window.title = "Prodigeetor"
    window.makeKeyAndOrderFront(nil)
    self.window = window

    setupMenu()
  }

  private func setupMenu() {
    let mainMenu = NSMenu()

    // App menu
    let appMenuItem = NSMenuItem()
    let appMenu = NSMenu()
    appMenu.addItem(withTitle: "Quit Prodigeetor", action: #selector(NSApplication.terminate(_:)), keyEquivalent: "q")
    appMenuItem.submenu = appMenu
    mainMenu.addItem(appMenuItem)

    // File menu
    let fileMenuItem = NSMenuItem()
    let fileMenu = NSMenu(title: "File")
    fileMenu.addItem(withTitle: "New Tab", action: #selector(newTab), keyEquivalent: "t")
    fileMenu.addItem(withTitle: "Open...", action: #selector(openFile), keyEquivalent: "o")
    fileMenu.addItem(withTitle: "Save", action: #selector(saveFile), keyEquivalent: "s")
    fileMenu.addItem(NSMenuItem.separator())
    fileMenu.addItem(withTitle: "Close Tab", action: #selector(closeTab), keyEquivalent: "w")
    fileMenuItem.submenu = fileMenu
    mainMenu.addItem(fileMenuItem)

    // View menu
    let viewMenuItem = NSMenuItem()
    let viewMenu = NSMenu(title: "View")

    let splitVerticalItem = NSMenuItem(title: "Split Editor Vertically", action: #selector(splitVertically), keyEquivalent: "\\")
    viewMenu.addItem(splitVerticalItem)

    let splitHorizontalItem = NSMenuItem(title: "Split Editor Horizontally", action: #selector(splitHorizontally), keyEquivalent: "\\")
    splitHorizontalItem.keyEquivalentModifierMask = [.command, .shift]
    viewMenu.addItem(splitHorizontalItem)

    viewMenu.addItem(NSMenuItem.separator())
    viewMenu.addItem(withTitle: "Close Split", action: #selector(closeSplit), keyEquivalent: "k")

    viewMenuItem.submenu = viewMenu
    mainMenu.addItem(viewMenuItem)

    // Window menu
    let windowMenuItem = NSMenuItem()
    let windowMenu = NSMenu(title: "Window")
    windowMenu.addItem(withTitle: "Minimize", action: #selector(NSWindow.miniaturize(_:)), keyEquivalent: "m")
    windowMenu.addItem(withTitle: "Zoom", action: #selector(NSWindow.zoom(_:)), keyEquivalent: "")
    windowMenu.addItem(NSMenuItem.separator())

    // Tab navigation
    windowMenu.addItem(withTitle: "Next Tab", action: #selector(nextTab), keyEquivalent: "]")
    windowMenu.addItem(withTitle: "Previous Tab", action: #selector(previousTab), keyEquivalent: "[")
    windowMenu.addItem(NSMenuItem.separator())

    // Tab selection by index (Cmd+1 through Cmd+9)
    for i in 1...9 {
      windowMenu.addItem(withTitle: "Select Tab \(i)", action: #selector(selectTabByIndex(_:)), keyEquivalent: "\(i)")
    }

    windowMenuItem.submenu = windowMenu
    mainMenu.addItem(windowMenuItem)

    NSApp.mainMenu = mainMenu
  }

  @objc private func newTab() {
    splitContainerViewController?.createNewTab()
  }

  @objc private func openFile() {
    splitContainerViewController?.openFile()
  }

  @objc private func saveFile() {
    splitContainerViewController?.saveActiveFile()
  }

  @objc private func closeTab() {
    splitContainerViewController?.closeActiveTab()
  }

  @objc private func splitVertically() {
    splitContainerViewController?.splitVertically()
  }

  @objc private func splitHorizontally() {
    splitContainerViewController?.splitHorizontally()
  }

  @objc private func closeSplit() {
    splitContainerViewController?.closeActiveSplitPane()
  }

  @objc private func nextTab() {
    splitContainerViewController?.selectNextTab()
  }

  @objc private func previousTab() {
    splitContainerViewController?.selectPreviousTab()
  }

  @objc private func selectTabByIndex(_ sender: NSMenuItem) {
    guard let keyEquivalent = Int(sender.keyEquivalent) else { return }
    splitContainerViewController?.selectTab(at: keyEquivalent - 1)
  }
}
