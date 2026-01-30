import AppKit

/// Represents a single tab item
struct TabItem {
  let id: UUID
  var title: String
  var isDirty: Bool
  var isActive: Bool
}

/// Custom tab bar view with VS Code-style tabs
final class TabBarView: NSView {
  private var tabs: [TabItem] = []
  private let tabHeight: CGFloat = 35
  private let tabMinWidth: CGFloat = 120
  private let tabMaxWidth: CGFloat = 200
  private var hoveredTabIndex: Int?
  private var hoveredCloseButton: Int?

  // Callbacks
  var onTabSelected: ((UUID) -> Void)?
  var onTabClosed: ((UUID) -> Void)?

  // Tracking area for mouse hover
  private var trackingArea: NSTrackingArea?

  override init(frame: NSRect) {
    super.init(frame: frame)
    setup()
  }

  required init?(coder: NSCoder) {
    super.init(coder: coder)
    setup()
  }

  private func setup() {
    wantsLayer = true
    layer?.backgroundColor = NSColor(red: 0.157, green: 0.173, blue: 0.212, alpha: 1.0).cgColor
  }

  override func updateTrackingAreas() {
    super.updateTrackingAreas()

    if let trackingArea = trackingArea {
      removeTrackingArea(trackingArea)
    }

    let options: NSTrackingArea.Options = [.mouseEnteredAndExited, .mouseMoved, .activeAlways]
    trackingArea = NSTrackingArea(rect: bounds, options: options, owner: self, userInfo: nil)
    if let trackingArea = trackingArea {
      addTrackingArea(trackingArea)
    }
  }

  // MARK: - Tab Management

  func addTab(id: UUID, title: String, isDirty: Bool = false) {
    let newTab = TabItem(id: id, title: title, isDirty: isDirty, isActive: false)
    tabs.append(newTab)
    needsDisplay = true
  }

  func removeTab(id: UUID) {
    tabs.removeAll { $0.id == id }
    needsDisplay = true
  }

  func updateTab(id: UUID, title: String? = nil, isDirty: Bool? = nil) {
    guard let index = tabs.firstIndex(where: { $0.id == id }) else { return }
    if let title = title {
      tabs[index].title = title
    }
    if let isDirty = isDirty {
      tabs[index].isDirty = isDirty
    }
    needsDisplay = true
  }

  func setActiveTab(id: UUID) {
    for index in tabs.indices {
      tabs[index].isActive = (tabs[index].id == id)
    }
    needsDisplay = true
  }

  func getActiveTabId() -> UUID? {
    return tabs.first(where: { $0.isActive })?.id
  }

  // MARK: - Drawing

  override func draw(_ dirtyRect: NSRect) {
    super.draw(dirtyRect)

    guard let context = NSGraphicsContext.current?.cgContext else { return }

    let tabWidth = min(max(bounds.width / CGFloat(max(tabs.count, 1)), tabMinWidth), tabMaxWidth)

    for (index, tab) in tabs.enumerated() {
      let tabRect = NSRect(
        x: CGFloat(index) * tabWidth,
        y: 0,
        width: tabWidth,
        height: tabHeight
      )

      drawTab(tab, in: tabRect, context: context, isHovered: hoveredTabIndex == index)
    }
  }

  private func drawTab(_ tab: TabItem, in rect: NSRect, context: CGContext, isHovered: Bool) {
    // Background
    let backgroundColor: NSColor
    if tab.isActive {
      backgroundColor = NSColor(red: 0.114, green: 0.129, blue: 0.161, alpha: 1.0)
    } else if isHovered {
      backgroundColor = NSColor(red: 0.18, green: 0.20, blue: 0.24, alpha: 1.0)
    } else {
      backgroundColor = NSColor(red: 0.157, green: 0.173, blue: 0.212, alpha: 1.0)
    }

    context.setFillColor(backgroundColor.cgColor)
    context.fill(rect)

    // Border
    if tab.isActive {
      context.setStrokeColor(NSColor(red: 0.29, green: 0.56, blue: 0.89, alpha: 1.0).cgColor)
      context.setLineWidth(2)
      context.move(to: CGPoint(x: rect.minX, y: rect.minY))
      context.addLine(to: CGPoint(x: rect.maxX, y: rect.minY))
      context.strokePath()
    }

    // Dirty indicator (dot)
    if tab.isDirty {
      let dotRadius: CGFloat = 3
      let dotX = rect.minX + 12
      let dotY = rect.midY

      context.setFillColor(NSColor.white.withAlphaComponent(0.8).cgColor)
      context.fillEllipse(in: NSRect(
        x: dotX - dotRadius,
        y: dotY - dotRadius,
        width: dotRadius * 2,
        height: dotRadius * 2
      ))
    }

    // Title
    let titleX = rect.minX + (tab.isDirty ? 26 : 12)
    let titleWidth = rect.width - (tab.isDirty ? 26 : 12) - 26 // Leave space for close button
    let titleRect = NSRect(
      x: titleX,
      y: rect.minY + 8,
      width: titleWidth,
      height: rect.height - 16
    )

    let paragraphStyle = NSMutableParagraphStyle()
    paragraphStyle.alignment = .left
    paragraphStyle.lineBreakMode = .byTruncatingTail

    let attributes: [NSAttributedString.Key: Any] = [
      .font: NSFont.systemFont(ofSize: 12),
      .foregroundColor: tab.isActive ? NSColor.white : NSColor.white.withAlphaComponent(0.7),
      .paragraphStyle: paragraphStyle
    ]

    let title = tab.title as NSString
    title.draw(in: titleRect, withAttributes: attributes)

    // Close button
    let closeButtonSize: CGFloat = 16
    let closeButtonRect = NSRect(
      x: rect.maxX - closeButtonSize - 8,
      y: rect.midY - closeButtonSize / 2,
      width: closeButtonSize,
      height: closeButtonSize
    )

    if isHovered || tab.isActive {
      // Draw close button background
      let closeButtonHovered = hoveredCloseButton == tabs.firstIndex(where: { $0.id == tab.id })
      if closeButtonHovered {
        context.setFillColor(NSColor.white.withAlphaComponent(0.2).cgColor)
        context.fillEllipse(in: closeButtonRect)
      }

      // Draw X
      context.setStrokeColor(NSColor.white.withAlphaComponent(0.7).cgColor)
      context.setLineWidth(1.5)
      let padding: CGFloat = 4
      context.move(to: CGPoint(x: closeButtonRect.minX + padding, y: closeButtonRect.minY + padding))
      context.addLine(to: CGPoint(x: closeButtonRect.maxX - padding, y: closeButtonRect.maxY - padding))
      context.move(to: CGPoint(x: closeButtonRect.maxX - padding, y: closeButtonRect.minY + padding))
      context.addLine(to: CGPoint(x: closeButtonRect.minX + padding, y: closeButtonRect.maxY - padding))
      context.strokePath()
    }
  }

  // MARK: - Mouse Events

  override func mouseDown(with event: NSEvent) {
    let location = convert(event.locationInWindow, from: nil)
    let tabWidth = min(max(bounds.width / CGFloat(max(tabs.count, 1)), tabMinWidth), tabMaxWidth)
    let tabIndex = Int(location.x / tabWidth)

    guard tabIndex >= 0 && tabIndex < tabs.count else { return }

    let tabRect = NSRect(
      x: CGFloat(tabIndex) * tabWidth,
      y: 0,
      width: tabWidth,
      height: tabHeight
    )

    // Check if close button was clicked
    let closeButtonSize: CGFloat = 16
    let closeButtonRect = NSRect(
      x: tabRect.maxX - closeButtonSize - 8,
      y: tabRect.midY - closeButtonSize / 2,
      width: closeButtonSize,
      height: closeButtonSize
    )

    if closeButtonRect.contains(location) {
      onTabClosed?(tabs[tabIndex].id)
    } else {
      onTabSelected?(tabs[tabIndex].id)
    }
  }

  override func mouseMoved(with event: NSEvent) {
    let location = convert(event.locationInWindow, from: nil)
    let tabWidth = min(max(bounds.width / CGFloat(max(tabs.count, 1)), tabMinWidth), tabMaxWidth)
    let tabIndex = Int(location.x / tabWidth)

    let previousHoveredTab = hoveredTabIndex
    let previousHoveredClose = hoveredCloseButton

    if tabIndex >= 0 && tabIndex < tabs.count {
      hoveredTabIndex = tabIndex

      let tabRect = NSRect(
        x: CGFloat(tabIndex) * tabWidth,
        y: 0,
        width: tabWidth,
        height: tabHeight
      )

      let closeButtonSize: CGFloat = 16
      let closeButtonRect = NSRect(
        x: tabRect.maxX - closeButtonSize - 8,
        y: tabRect.midY - closeButtonSize / 2,
        width: closeButtonSize,
        height: closeButtonSize
      )

      hoveredCloseButton = closeButtonRect.contains(location) ? tabIndex : nil
    } else {
      hoveredTabIndex = nil
      hoveredCloseButton = nil
    }

    if previousHoveredTab != hoveredTabIndex || previousHoveredClose != hoveredCloseButton {
      needsDisplay = true
    }
  }

  override func mouseExited(with event: NSEvent) {
    hoveredTabIndex = nil
    hoveredCloseButton = nil
    needsDisplay = true
  }

  override var intrinsicContentSize: NSSize {
    return NSSize(width: NSView.noIntrinsicMetric, height: tabHeight)
  }
}
