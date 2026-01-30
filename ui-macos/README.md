# macOS UI Shell

This folder is intended to host the Xcode project for the macOS AppKit shell.

Expected project path for build scripts:
- ui-macos/Prodigeetor.xcodeproj
- Scheme: Prodigeetor

Notes:
- The project includes an Objective-C++ bridge in `ui-macos/Prodigeetor/Bridge/CoreBridge.mm`.
- Link the core static library (built via CMake) once it exists in your build pipeline.
