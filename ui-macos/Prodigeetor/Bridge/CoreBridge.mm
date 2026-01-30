#import "CoreBridge.h"

#include "core.h"

#include <memory>
#include <string>

@interface CoreBridge () {
  std::unique_ptr<prodigeetor::Core> _core;
}
@end

@implementation CoreBridge

- (instancetype)init {
  self = [super init];
  if (self) {
    _core = std::make_unique<prodigeetor::Core>();
  }
  return self;
}

- (void)initializeCore {
  if (_core) {
    _core->initialize();
  }
}

- (void)initializeLSPWithWorkspace:(NSString *)workspacePath {
  if (!_core) {
    return;
  }
  std::string path = std::string([workspacePath UTF8String]);
  _core->initialize_lsp(path);
}

- (void)tick {
  if (_core) {
    _core->tick();
  }
}

- (void)openFile:(NSString *)uri languageId:(NSString *)languageId {
  if (!_core) {
    return;
  }
  std::string uriStr = std::string([uri UTF8String]);
  std::string langStr = std::string([languageId UTF8String]);
  _core->open_file(uriStr, langStr);
}

- (void)closeFile:(NSString *)uri {
  if (!_core) {
    return;
  }
  std::string uriStr = std::string([uri UTF8String]);
  _core->close_file(uriStr);
}

- (void)saveFile:(NSString *)uri {
  if (!_core) {
    return;
  }
  std::string uriStr = std::string([uri UTF8String]);
  _core->save_file(uriStr);
}

- (void)didChangeFile:(NSString *)uri {
  if (!_core) {
    return;
  }
  std::string uriStr = std::string([uri UTF8String]);
  std::string text = _core->buffer().text();
  _core->lsp_manager().didChange(uriStr, text);
}

- (void)setText:(NSString *)text {
  if (!_core) {
    return;
  }
  std::string value;
  if (text) {
    value = std::string([text UTF8String]);
  }
  _core->set_text(std::move(value));
}

- (NSString *)getText {
  if (!_core) {
    return @"";
  }
  std::string text = _core->buffer().text();
  return [NSString stringWithUTF8String:text.c_str()];
}

- (NSInteger)lineCount {
  if (!_core) {
    return 0;
  }
  return static_cast<NSInteger>(_core->line_count());
}

- (NSString *)lineTextAt:(NSInteger)lineIndex {
  if (!_core) {
    return @"";
  }
  std::string line = _core->line_text(static_cast<size_t>(lineIndex));
  return [NSString stringWithUTF8String:line.c_str()];
}

- (NSInteger)lineGraphemeCount:(NSInteger)lineIndex {
  if (!_core) {
    return 0;
  }
  return static_cast<NSInteger>(_core->line_grapheme_count(static_cast<size_t>(lineIndex)));
}

- (NSInteger)insertText:(NSString *)text atOffset:(NSInteger)offset {
  if (!_core) {
    return offset;
  }
  std::string value;
  if (text) {
    value = std::string([text UTF8String]);
  }
  size_t pos = static_cast<size_t>(offset);
  _core->insert(pos, value);
  return static_cast<NSInteger>(pos + value.size());
}

- (NSInteger)deleteBackwardFromOffset:(NSInteger)offset {
  if (!_core) {
    return offset;
  }
  size_t pos = static_cast<size_t>(offset);
  size_t new_offset = _core->delete_backward(pos);
  return static_cast<NSInteger>(new_offset);
}

- (NSArray<NSNumber *> *)positionAtOffset:(NSInteger)offset {
  if (!_core) {
    return @[@0, @0];
  }
  prodigeetor::Position pos = _core->position_at(static_cast<size_t>(offset));
  return @[@(pos.line), @(pos.column)];
}

- (NSInteger)offsetAtLine:(NSInteger)line column:(NSInteger)column {
  if (!_core) {
    return 0;
  }
  prodigeetor::Position pos{static_cast<uint32_t>(line), static_cast<uint32_t>(column)};
  return static_cast<NSInteger>(_core->offset_at(pos));
}

@end
