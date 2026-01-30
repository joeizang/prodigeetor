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

target_link_libraries(prodigeetor_core PRIVATE tree_sitter tree_sitter_javascript)
target_compile_definitions(prodigeetor_core PRIVATE PRODIGEETOR_USE_TREE_SITTER=1)

# Placeholder for future dependencies (tree-sitter, JSON, etc.)
