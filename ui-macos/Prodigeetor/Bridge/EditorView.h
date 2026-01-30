#import <AppKit/AppKit.h>

@class CoreBridge;

NS_ASSUME_NONNULL_BEGIN

@interface EditorView : NSView

- (instancetype)initWithFrame:(NSRect)frameRect coreBridge:(CoreBridge *)coreBridge;
- (void)setEditorFont:(NSString *)family size:(CGFloat)size;

@end

NS_ASSUME_NONNULL_END
