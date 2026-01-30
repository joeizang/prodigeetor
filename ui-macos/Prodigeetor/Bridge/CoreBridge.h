#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface CoreBridge : NSObject

- (instancetype)init;
- (void)initializeCore;
- (void)setText:(NSString *)text;
- (NSString *)getText;
- (NSInteger)lineCount;
- (NSString *)lineTextAt:(NSInteger)lineIndex;
- (NSInteger)lineGraphemeCount:(NSInteger)lineIndex;
- (NSInteger)insertText:(NSString *)text atOffset:(NSInteger)offset;
- (NSInteger)deleteBackwardFromOffset:(NSInteger)offset;
- (NSArray<NSNumber *> *)positionAtOffset:(NSInteger)offset;
- (NSInteger)offsetAtLine:(NSInteger)line column:(NSInteger)column;

@end

NS_ASSUME_NONNULL_END
