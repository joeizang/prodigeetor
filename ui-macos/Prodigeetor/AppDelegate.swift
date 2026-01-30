import AppKit

@main
final class AppDelegate: NSObject, NSApplicationDelegate {
  private var window: NSWindow?

  func applicationDidFinishLaunching(_ notification: Notification) {
    let viewController = MainViewController()
    let window = NSWindow(contentViewController: viewController)
    window.setContentSize(NSSize(width: 960, height: 640))
    window.title = "Prodigeetor"
    window.makeKeyAndOrderFront(nil)
    self.window = window
  }
}
