import AppKit

let app = NSApplication.shared
app.setActivationPolicy(.regular)
let delegate = AppDelegate()
app.delegate = delegate
app.activate(ignoringOtherApps: true)

// Manually trigger the finish launching sequence
NotificationCenter.default.post(
    name: NSApplication.didFinishLaunchingNotification,
    object: app
)

app.run()
