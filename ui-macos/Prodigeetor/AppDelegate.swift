import AppKit

final class AppDelegate: NSObject, NSApplicationDelegate {
  private var window: NSWindow?
  private var mainViewController: MainViewController?

  func applicationDidFinishLaunching(_ notification: Notification) {
    NSApp.setActivationPolicy(.regular)
    NSApp.activate(ignoringOtherApps: true)

    let viewController = MainViewController()
    self.mainViewController = viewController

    let window = NSWindow(contentViewController: viewController)
    window.styleMask.insert([.titled, .closable, .miniaturizable, .resizable])
    window.isReleasedWhenClosed = false
    window.backgroundColor = NSColor.windowBackgroundColor

    window.setContentSize(NSSize(width: 960, height: 640))
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
    fileMenu.addItem(withTitle: "Open...", action: #selector(openFile), keyEquivalent: "o")
    fileMenu.addItem(withTitle: "Save", action: #selector(saveFile), keyEquivalent: "s")
    fileMenuItem.submenu = fileMenu
    mainMenu.addItem(fileMenuItem)

    // Window menu
    let windowMenuItem = NSMenuItem()
    let windowMenu = NSMenu(title: "Window")
    windowMenu.addItem(withTitle: "Minimize", action: #selector(NSWindow.miniaturize(_:)), keyEquivalent: "m")
    windowMenu.addItem(withTitle: "Zoom", action: #selector(NSWindow.zoom(_:)), keyEquivalent: "")
    windowMenuItem.submenu = windowMenu
    mainMenu.addItem(windowMenuItem)

    NSApp.mainMenu = mainMenu
  }

  @objc private func openFile() {
    mainViewController?.openFile()
  }

  @objc private func saveFile() {
    mainViewController?.saveFile()
  }
}
