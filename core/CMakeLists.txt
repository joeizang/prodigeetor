cmake_minimum_required(VERSION 3.20)
project(prodigeetor_core LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(prodigeetor_core STATIC
  src/core.cpp
  src/text_buffer.cpp
  src/undo_stack.cpp
  src/rendering.cpp
  src/grapheme.cpp
  src/syntax_highlighter.cpp
  src/theme.cpp
  src/settings.cpp
  src/lsp_client.cpp
  src/lsp_manager.cpp
)

target_include_directories(prodigeetor_core PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

set(UTF8PROC_INSTALL OFF CACHE BOOL "" FORCE)
set(UTF8PROC_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../third_party/utf8proc
  ${CMAKE_CURRENT_BINARY_DIR}/utf8proc
  EXCLUDE_FROM_ALL)
target_link_libraries(prodigeetor_core PRIVATE utf8proc)
target_compile_definitions(prodigeetor_core PRIVATE PRODIGEETOR_USE_UTF8PROC=1)

add_library(tree_sitter STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/tree-sitter/lib/src/lib.c
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/tree-sitter/lib/src/parser.c
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/tree-sitter/lib/src/lexer.c
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/tree-sitter/lib/src/node.c
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/tree-sitter/lib/src/tree.c
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/tree-sitter/lib/src/tree_cursor.c
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/tree-sitter/lib/src/language.c
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/tree-sitter/lib/src/query.c
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/tree-sitter/lib/src/stack.c
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/tree-sitter/lib/src/get_changed_ranges.c
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/tree-sitter/lib/src/subtree.c
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/tree-sitter/lib/src/alloc.c
)
target_include_directories(tree_sitter PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/tree-sitter/lib/include
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/tree-sitter/lib/src
)

add_library(tree_sitter_javascript STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/tree-sitter-javascript/src/parser.c
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/tree-sitter-javascript/src/scanner.c
)
target_include_directories(tree_sitter_javascript PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/tree-sitter/lib/include
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/tree-sitter-javascript/src
)

add_library(tree_sitter_typescript STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/tree-sitter-typescript/typescript/src/parser.c
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/tree-sitter-typescript/typescript/src/scanner.c
)
target_include_directories(tree_sitter_typescript PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/tree-sitter/lib/include
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/tree-sitter-typescript/typescript/src
)

add_library(tree_sitter_tsx STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/tree-sitter-typescript/tsx/src/parser.c
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/tree-sitter-typescript/tsx/src/scanner.c
)
target_include_directories(tree_sitter_tsx PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/tree-sitter/lib/include
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/tree-sitter-typescript/tsx/src
)

add_library(tree_sitter_swift STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/tree-sitter-swift/src/parser.c
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/tree-sitter-swift/src/scanner.c
)
target_include_directories(tree_sitter_swift PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/tree-sitter/lib/include
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/tree-sitter-swift/src
)

add_library(tree_sitter_c_sharp STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/tree-sitter-c-sharp/src/parser.c
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/tree-sitter-c-sharp/src/scanner.c
)
target_include_directories(tree_sitter_c_sharp PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/tree-sitter/lib/include
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/tree-sitter-c-sharp/src
)

add_library(tree_sitter_html STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/tree-sitter-html/src/parser.c
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/tree-sitter-html/src/scanner.c
)
target_include_directories(tree_sitter_html PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/tree-sitter/lib/include
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/tree-sitter-html/src
)

add_library(tree_sitter_css STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/tree-sitter-css/src/parser.c
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/tree-sitter-css/src/scanner.c
)
target_include_directories(tree_sitter_css PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/tree-sitter/lib/include
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/tree-sitter-css/src
)

add_library(tree_sitter_sql STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/tree-sitter-sql/src/parser.c
)
target_include_directories(tree_sitter_sql PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/tree-sitter/lib/include
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/tree-sitter-sql/src
)

target_link_libraries(prodigeetor_core PRIVATE
  tree_sitter
  tree_sitter_javascript
  tree_sitter_typescript
  tree_sitter_tsx
  tree_sitter_swift
  tree_sitter_c_sharp
  tree_sitter_html
  tree_sitter_css
  tree_sitter_sql
)
target_compile_definitions(prodigeetor_core PRIVATE PRODIGEETOR_USE_TREE_SITTER=1)

# Placeholder for future dependencies (tree-sitter, JSON, etc.)
