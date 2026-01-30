#import "EditorView.h"
#import "CoreBridge.h"

#include <string>
#include <vector>

#include "CoreTextRenderer.h"
#include "settings.h"
#include "syntax_highlighter.h"
#include "theme.h"

// Helper function to resolve resource paths from bundle or working directory
static std::string resolveResourcePath(NSString *relativePath) {
  // First try to load from app bundle Resources
  NSString *filename = [relativePath lastPathComponent];
  NSString *directory = [relativePath stringByDeletingLastPathComponent];

  NSString *bundlePath = [[NSBundle mainBundle] pathForResource:[filename stringByDeletingPathExtension]
                                                          ofType:[filename pathExtension]
                                                     inDirectory:directory];

  if (bundlePath && [[NSFileManager defaultManager] fileExistsAtPath:bundlePath]) {
    return std::string([bundlePath UTF8String]);
  }

  // Fall back to working directory (for development)
  return std::string([relativePath UTF8String]);
}

@interface EditorView ()
@property (nonatomic, strong) CoreBridge *coreBridge;
@end

@implementation EditorView {
  prodigeetor::CoreTextRenderer _renderer;
  prodigeetor::TreeSitterHighlighter _highlighter;
  NSString *_fontFamily;
  CGFloat _fontSize;
  NSInteger _cursorOffset;
  NSInteger _selectionAnchor;
  CGFloat _lineHeight;
  CGFloat _baseline;
  BOOL _isDragging;
  CGFloat _scrollOffsetY;
  NSString *_filePath;
  NSString *_themePath;
  NSDate *_themeLastModified;
  NSTimer *_themeTimer;
  NSTimer *_displayTimer;
  NSDate *_lastBlink;
  NSTimeInterval _blinkInterval;
  BOOL _caretVisible;
  prodigeetor::EditorSettings _settings;
}

- (instancetype)initWithFrame:(NSRect)frameRect coreBridge:(CoreBridge *)coreBridge {
  self = [super initWithFrame:frameRect];
  if (self) {
    _coreBridge = coreBridge;
    std::string settingsPath = resolveResourcePath(@"settings/default.json");
    _settings = prodigeetor::SettingsLoader::load_from_file(settingsPath);
    _fontFamily = [NSString stringWithUTF8String:_settings.font_family.c_str()];
    _fontSize = 14.0;
    _cursorOffset = 0;
    _selectionAnchor = 0;
    _lineHeight = 18.0;
    _baseline = 14.0;
    _isDragging = NO;
    _scrollOffsetY = 0.0;
    [self setWantsLayer:YES];
    _blinkInterval = 0.5;
    _caretVisible = YES;
    _lastBlink = [NSDate date];

    _themePath = @"themes/default.json";
    [self reloadThemeIfNeeded:YES];
    _highlighter.set_language(prodigeetor::TreeSitterHighlighter::LanguageId::JavaScript);

    std::vector<std::string> families;
    families.push_back(_settings.font_family);
    for (const auto &fallback : _settings.font_fallbacks) {
      families.push_back(fallback);
    }
    _renderer.set_font_stack(families, _settings.font_size);
    _renderer.set_ligatures(_settings.font_ligatures);
    _fontSize = _settings.font_size;

    // Set initial frame size if needed
    if (NSEqualRects(frameRect, NSZeroRect)) {
      self.frame = NSMakeRect(0, 0, 800, 600);
    }

    [self setNeedsDisplay:YES];
  }
  return self;
}

- (BOOL)isFlipped {
  return YES;
}

- (BOOL)acceptsFirstResponder {
  return YES;
}

- (void)dealloc {
  [_themeTimer invalidate];
  _themeTimer = nil;
  [_displayTimer invalidate];
  _displayTimer = nil;
  [super dealloc];
}

- (void)viewDidMoveToWindow {
  [super viewDidMoveToWindow];
  if (self.window) {
    [self startDisplayLoop];
  } else {
    [self stopDisplayLoop];
  }
}

- (void)startDisplayLoop {
  if (_displayTimer) {
    return;
  }
  _displayTimer = [NSTimer scheduledTimerWithTimeInterval:(1.0 / 60.0)
                                                   target:self
                                                 selector:@selector(onDisplayTick)
                                                    repeats:YES
                                                   userInfo:nil];
}

- (void)stopDisplayLoop {
  [_displayTimer invalidate];
  _displayTimer = nil;
}

- (void)onDisplayTick {
  if (!self.window) {
    return;
  }
  NSDate *now = [NSDate date];
  if ([now timeIntervalSinceDate:_lastBlink] >= _blinkInterval) {
    _caretVisible = !_caretVisible;
    _lastBlink = now;
  }
  [self setNeedsDisplay:YES];
}

- (void)setEditorFont:(NSString *)family size:(CGFloat)size {
  _fontFamily = family ?: @"Monoid";
  _fontSize = size > 0 ? size : 14.0;
  std::vector<std::string> families;
  families.push_back([_fontFamily UTF8String]);
  for (const auto &fallback : _settings.font_fallbacks) {
    families.push_back(fallback);
  }
  _renderer.set_font_stack(families, static_cast<float>(_fontSize));
  _renderer.set_ligatures(_settings.font_ligatures);
  prodigeetor::LayoutMetrics metrics = _renderer.measure_line("M");
  _lineHeight = metrics.height > 0 ? metrics.height : 18.0;
  _baseline = metrics.baseline > 0 ? metrics.baseline : 14.0;
  [self setNeedsDisplay:YES];
}

- (void)setFilePath:(NSString *)path {
  _filePath = [path copy];
  if (!_filePath) {
    return;
  }
  NSString *lower = _filePath.lowercaseString;
  if ([lower hasSuffix:@".ts"]) {
    _highlighter.set_language(prodigeetor::TreeSitterHighlighter::LanguageId::TypeScript);
  } else if ([lower hasSuffix:@".tsx"]) {
    _highlighter.set_language(prodigeetor::TreeSitterHighlighter::LanguageId::TSX);
  } else if ([lower hasSuffix:@".js"] || [lower hasSuffix:@".jsx"]) {
    _highlighter.set_language(prodigeetor::TreeSitterHighlighter::LanguageId::JavaScript);
  } else if ([lower hasSuffix:@".swift"]) {
    _highlighter.set_language(prodigeetor::TreeSitterHighlighter::LanguageId::Swift);
  } else if ([lower hasSuffix:@".cs"]) {
    _highlighter.set_language(prodigeetor::TreeSitterHighlighter::LanguageId::CSharp);
  } else if ([lower hasSuffix:@".html"] || [lower hasSuffix:@".htm"]) {
    _highlighter.set_language(prodigeetor::TreeSitterHighlighter::LanguageId::HTML);
  } else if ([lower hasSuffix:@".css"]) {
    _highlighter.set_language(prodigeetor::TreeSitterHighlighter::LanguageId::CSS);
  } else if ([lower hasSuffix:@".sql"]) {
    _highlighter.set_language(prodigeetor::TreeSitterHighlighter::LanguageId::SQL);
  } else {
    _highlighter.set_language(prodigeetor::TreeSitterHighlighter::LanguageId::JavaScript);
  }
  [self setNeedsDisplay:YES];
}

- (void)setThemePath:(NSString *)path {
  _themePath = [path copy];
  [self reloadThemeIfNeeded:YES];
}

- (void)reloadThemeIfNeeded:(BOOL)force {
  if (!_themePath) {
    return;
  }
  NSString *expanded = [_themePath stringByStandardizingPath];
  NSDictionary<NSFileAttributeKey, id> *attrs = [[NSFileManager defaultManager] attributesOfItemAtPath:expanded error:nil];
  NSDate *modified = attrs.fileModificationDate;
  if (!force && _themeLastModified && modified && [modified compare:_themeLastModified] != NSOrderedDescending) {
    return;
  }
  _themeLastModified = modified;
  std::string themePath = resolveResourcePath(_themePath);
  prodigeetor::SyntaxTheme theme = prodigeetor::SyntaxTheme::load_from_file(themePath);
  _highlighter.set_theme(std::move(theme));
  [self setNeedsDisplay:YES];

  if (!_themeTimer) {
    _themeTimer = [NSTimer scheduledTimerWithTimeInterval:1.0
                                                   target:self
                                                 selector:@selector(onThemeTimer)
                                                 userInfo:nil
                                                  repeats:YES];
  }
}

- (void)onThemeTimer {
  [self reloadThemeIfNeeded:NO];
}

- (NSString *)substringByGraphemeCount:(NSString *)text count:(NSInteger)count {
  if (count <= 0) {
    return @"";
  }
  __block NSInteger remaining = count;
  __block NSRange range = NSMakeRange(0, 0);
  [text enumerateSubstringsInRange:NSMakeRange(0, text.length)
                           options:NSStringEnumerationByComposedCharacterSequences
                        usingBlock:^(__unused NSString *substring, NSRange substringRange, __unused NSRange enclosingRange, BOOL *stop) {
    if (remaining <= 0) {
      *stop = YES;
      return;
    }
    range.length = NSMaxRange(substringRange);
    remaining -= 1;
    if (remaining == 0) {
      *stop = YES;
    }
  }];
  if (range.length == 0) {
    return @"";
  }
  return [text substringWithRange:NSMakeRange(0, range.length)];
}

- (NSInteger)columnForX:(CGFloat)x inLine:(NSString *)line {
  CGFloat localX = x - 8.0;
  if (localX <= 0) {
    return 0;
  }
  __block NSInteger column = 0;
  __block NSInteger graphemeIndex = 0;
  [line enumerateSubstringsInRange:NSMakeRange(0, line.length)
                           options:NSStringEnumerationByComposedCharacterSequences
                        usingBlock:^(__unused NSString *substring, NSRange substringRange, __unused NSRange enclosingRange, BOOL *stop) {
    graphemeIndex += 1;
    NSString *prefix = [line substringWithRange:NSMakeRange(0, NSMaxRange(substringRange))];
    std::string prefixUtf8 = std::string([prefix UTF8String]);
    prodigeetor::LayoutMetrics metrics = _renderer.measure_line(prefixUtf8);
    if (metrics.width >= localX) {
      column = graphemeIndex - 1;
      *stop = YES;
    } else {
      column = graphemeIndex;
    }
  }];
  return column;
}

- (void)updateSelectionWithCursor:(BOOL)extendSelection {
  if (!extendSelection) {
    _selectionAnchor = _cursorOffset;
  }
  [self setNeedsDisplay:YES];
}

- (CGFloat)maxScrollOffset {
  NSInteger lineCount = [self.coreBridge lineCount];
  CGFloat contentHeight = lineCount * _lineHeight + 16.0;
  CGFloat viewHeight = self.bounds.size.height;
  if (contentHeight <= viewHeight) {
    return 0.0;
  }
  return contentHeight - viewHeight;
}

- (void)updateCursorFromPoint:(NSPoint)point extendSelection:(BOOL)extendSelection {
  CGFloat contentY = point.y + _scrollOffsetY;
  NSInteger lineIndex = (NSInteger)floor((contentY - 8.0) / _lineHeight);
  if (lineIndex < 0) {
    lineIndex = 0;
  }
  NSInteger lineCount = [self.coreBridge lineCount];
  if (lineCount == 0) {
    _cursorOffset = 0;
    [self updateSelectionWithCursor:extendSelection];
    return;
  }
  if (lineIndex >= lineCount) {
    lineIndex = lineCount - 1;
  }
  NSString *line = [self.coreBridge lineTextAt:lineIndex];
  NSInteger column = [self columnForX:point.x inLine:line];
  _cursorOffset = [self.coreBridge offsetAtLine:lineIndex column:column];
  [self updateSelectionWithCursor:extendSelection];
}

- (void)mouseDown:(NSEvent *)event {
  [self.window makeFirstResponder:self];
  NSPoint point = [self convertPoint:event.locationInWindow fromView:nil];
  BOOL extend = (event.modifierFlags & NSEventModifierFlagShift) == NSEventModifierFlagShift;
  if (!extend) {
    _selectionAnchor = _cursorOffset;
  }
  [self updateCursorFromPoint:point extendSelection:extend];
  _isDragging = YES;
}

- (void)mouseDragged:(NSEvent *)event {
  if (!_isDragging) {
    return;
  }
  NSPoint point = [self convertPoint:event.locationInWindow fromView:nil];
  CGFloat maxScroll = [self maxScrollOffset];
  if (point.y < 0.0) {
    _scrollOffsetY = MAX(0.0, _scrollOffsetY - _lineHeight);
    point.y = 0.0;
  } else if (point.y > self.bounds.size.height) {
    _scrollOffsetY = MIN(maxScroll, _scrollOffsetY + _lineHeight);
    point.y = self.bounds.size.height;
  }
  if (self.enclosingScrollView) {
    NSPoint origin = self.enclosingScrollView.contentView.bounds.origin;
    origin.y = _scrollOffsetY;
    [self.enclosingScrollView.contentView scrollToPoint:origin];
    [self.enclosingScrollView reflectScrolledClipView:self.enclosingScrollView.contentView];
  }
  [self updateCursorFromPoint:point extendSelection:YES];
}

- (void)mouseUp:(NSEvent *)event {
  (void)event;
  _isDragging = NO;
}

- (void)moveCursorLeft:(BOOL)extendSelection {
  NSArray<NSNumber *> *pos = [self.coreBridge positionAtOffset:_cursorOffset];
  NSInteger line = pos[0].integerValue;
  NSInteger column = pos[1].integerValue;
  if (column > 0) {
    column -= 1;
  } else if (line > 0) {
    line -= 1;
    column = [self.coreBridge lineGraphemeCount:line];
  }
  _cursorOffset = [self.coreBridge offsetAtLine:line column:column];
  [self updateSelectionWithCursor:extendSelection];
}

- (void)moveCursorRight:(BOOL)extendSelection {
  NSArray<NSNumber *> *pos = [self.coreBridge positionAtOffset:_cursorOffset];
  NSInteger line = pos[0].integerValue;
  NSInteger column = pos[1].integerValue;
  NSInteger lineCount = [self.coreBridge lineCount];
  NSInteger lineColumns = [self.coreBridge lineGraphemeCount:line];
  if (column < lineColumns) {
    column += 1;
  } else if (line + 1 < lineCount) {
    line += 1;
    column = 0;
  }
  _cursorOffset = [self.coreBridge offsetAtLine:line column:column];
  [self updateSelectionWithCursor:extendSelection];
}

- (void)keyDown:(NSEvent *)event {
  BOOL extend = (event.modifierFlags & NSEventModifierFlagShift) == NSEventModifierFlagShift;
  if ((event.modifierFlags & NSEventModifierFlagCommand) == NSEventModifierFlagCommand) {
    if ([event.characters.lowercaseString isEqualToString:@"r"]) {
      [self reloadThemeIfNeeded:YES];
      return;
    }
  }
  switch (event.keyCode) {
    case 123: // left arrow
      [self moveCursorLeft:extend];
      return;
    case 124: // right arrow
      [self moveCursorRight:extend];
      return;
    case 51: // delete/backspace
      _cursorOffset = [self.coreBridge deleteBackwardFromOffset:_cursorOffset];
      [self updateSelectionWithCursor:NO];
      return;
    default:
      break;
  }
  NSString *characters = event.characters;
  if (characters.length > 0) {
    _cursorOffset = [self.coreBridge insertText:characters atOffset:_cursorOffset];
    [self updateSelectionWithCursor:NO];
    return;
  }
  [super keyDown:event];
}

- (void)drawSelectionForLine:(NSInteger)lineIndex lineText:(NSString *)line y:(CGFloat)y {
  NSInteger selectionStart = MIN(_cursorOffset, _selectionAnchor);
  NSInteger selectionEnd = MAX(_cursorOffset, _selectionAnchor);
  if (selectionStart == selectionEnd) {
    return;
  }
  NSInteger lineStartOffset = [self.coreBridge offsetAtLine:lineIndex column:0];
  NSInteger lineEndOffset = [self.coreBridge offsetAtLine:lineIndex column:[self.coreBridge lineGraphemeCount:lineIndex]];
  if (selectionEnd <= lineStartOffset || selectionStart >= lineEndOffset) {
    return;
  }

  NSArray<NSNumber *> *startPos = [self.coreBridge positionAtOffset:selectionStart];
  NSArray<NSNumber *> *endPos = [self.coreBridge positionAtOffset:selectionEnd];

  NSInteger startColumn = (startPos[0].integerValue == lineIndex) ? startPos[1].integerValue : 0;
  NSInteger endColumn = (endPos[0].integerValue == lineIndex) ? endPos[1].integerValue : [self.coreBridge lineGraphemeCount:lineIndex];

  NSString *prefixStart = [self substringByGraphemeCount:line count:startColumn];
  NSString *prefixEnd = [self substringByGraphemeCount:line count:endColumn];

  float xStart = 8.0f + _renderer.measure_line(std::string([prefixStart UTF8String])).width;
  float xEnd = 8.0f + _renderer.measure_line(std::string([prefixEnd UTF8String])).width;
  if (xEnd < xStart) {
    float temp = xStart;
    xStart = xEnd;
    xEnd = temp;
  }

  NSRect selectionRect = NSMakeRect(xStart, y, xEnd - xStart, _lineHeight);
  [[NSColor selectedTextBackgroundColor] setFill];
  NSRectFillUsingOperation(selectionRect, NSCompositingOperationSourceOver);
}

- (void)drawCaretForLine:(NSInteger)lineIndex lineText:(NSString *)line y:(CGFloat)y {
  if (!_caretVisible) {
    return;
  }
  if (self.window.firstResponder != self) {
    return;
  }
  NSArray<NSNumber *> *pos = [self.coreBridge positionAtOffset:_cursorOffset];
  if (pos[0].integerValue != lineIndex) {
    return;
  }
  NSInteger column = pos[1].integerValue;
  NSString *prefix = [self substringByGraphemeCount:line count:column];
  float x = 8.0f + _renderer.measure_line(std::string([prefix UTF8String])).width;
  NSRect caret = NSMakeRect(x, y, 1.0, _lineHeight);
  [[NSColor textColor] setFill];
  NSRectFill(caret);
}

- (void)drawRect:(NSRect)dirtyRect {
  [super drawRect:dirtyRect];

  // Draw background - YukiNord background color #1D2129
  [[NSColor colorWithRed:0.114 green:0.129 blue:0.161 alpha:1.0] setFill];
  NSRectFill(dirtyRect);

  CGContextRef context = [[NSGraphicsContext currentContext] CGContext];
  if (!context) {
    return;
  }

  _renderer.set_context(context);
  prodigeetor::LayoutMetrics metrics = _renderer.measure_line("M");
  _lineHeight = metrics.height > 0 ? metrics.height : _lineHeight;
  _baseline = metrics.baseline > 0 ? metrics.baseline : _baseline;

  NSInteger lineCount = [self.coreBridge lineCount];
  CGFloat contentHeight = lineCount * _lineHeight + 16.0;
  if (self.frame.size.height != contentHeight) {
    self.frame = NSMakeRect(self.frame.origin.x, self.frame.origin.y, self.frame.size.width, contentHeight);
  }

  _scrollOffsetY = self.bounds.origin.y;
  NSInteger startLine = (NSInteger)floor(_scrollOffsetY / _lineHeight);
  CGFloat offset = _scrollOffsetY - (startLine * _lineHeight);
  CGFloat y = 8.0 - offset;
  for (NSInteger i = startLine; i < lineCount && y < self.bounds.size.height; i++) {
    NSString *line = [self.coreBridge lineTextAt:i];
    [self drawSelectionForLine:i lineText:line y:y];

    std::string lineText = std::string([line UTF8String]);
    std::vector<prodigeetor::RenderSpan> spans = _highlighter.highlight(lineText);
    prodigeetor::LineLayout layout = _renderer.layout_line(lineText, spans);
    _renderer.draw_line(layout, 8.0f, static_cast<float>(y));
    [self drawCaretForLine:i lineText:line y:y];
    y += _lineHeight;
  }
}

@end
