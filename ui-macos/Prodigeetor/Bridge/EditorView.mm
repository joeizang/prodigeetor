#import "EditorView.h"
#import "CoreBridge.h"

#include <string>
#include <vector>

#include "CoreTextRenderer.h"
#include "syntax_highlighter.h"

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
}

- (instancetype)initWithFrame:(NSRect)frameRect coreBridge:(CoreBridge *)coreBridge {
  self = [super initWithFrame:frameRect];
  if (self) {
    _coreBridge = coreBridge;
    _fontFamily = @"Menlo";
    _fontSize = 14.0;
    _cursorOffset = 0;
    _selectionAnchor = 0;
    _lineHeight = 18.0;
    _baseline = 14.0;
    _isDragging = NO;
    [self setWantsLayer:YES];
  }
  return self;
}

- (BOOL)isFlipped {
  return YES;
}

- (BOOL)acceptsFirstResponder {
  return YES;
}

- (void)setEditorFont:(NSString *)family size:(CGFloat)size {
  _fontFamily = family ?: @"Menlo";
  _fontSize = size > 0 ? size : 14.0;
  _renderer.set_font([_fontFamily UTF8String], static_cast<float>(_fontSize));
  prodigeetor::LayoutMetrics metrics = _renderer.measure_line("M");
  _lineHeight = metrics.height > 0 ? metrics.height : 18.0;
  _baseline = metrics.baseline > 0 ? metrics.baseline : 14.0;
  [self setNeedsDisplay:YES];
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

- (void)updateCursorFromPoint:(NSPoint)point extendSelection:(BOOL)extendSelection {
  NSInteger lineIndex = (NSInteger)floor((point.y - 8.0) / _lineHeight);
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

  CGContextRef context = [[NSGraphicsContext currentContext] CGContext];
  if (!context) {
    return;
  }

  _renderer.set_context(context);
  _renderer.set_font([_fontFamily UTF8String], static_cast<float>(_fontSize));
  prodigeetor::LayoutMetrics metrics = _renderer.measure_line("M");
  _lineHeight = metrics.height > 0 ? metrics.height : _lineHeight;
  _baseline = metrics.baseline > 0 ? metrics.baseline : _baseline;

  NSInteger lineCount = [self.coreBridge lineCount];
  CGFloat y = 8.0;
  for (NSInteger i = 0; i < lineCount; i++) {
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
