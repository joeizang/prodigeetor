cmake_minimum_required(VERSION 3.20)
project(prodigeetor_ui_linux LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED IMPORTED_TARGET gtk4)

add_executable(prodigeetor
  src/main.cpp
  src/pango_renderer.cpp
  src/editor_widget.cpp
  src/tab_container.cpp
  src/split_container.cpp
)

target_link_libraries(prodigeetor PRIVATE
  prodigeetor_core
  PkgConfig::GTK4
)

target_include_directories(prodigeetor PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/include
)
