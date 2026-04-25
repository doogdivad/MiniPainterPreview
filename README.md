# Mini Painter Core (Skeleton)

This repository contains a portable C++ core library scaffold for Mini Painter.

## Build

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Targets

- `mini_painter_core`: static library exposing a stable C API wrapper.
- `mini_painter_cli`: minimal CLI tool.
- `mini_painter_tests`: basic executable test for project create/open/save.

## Implemented in this milestone

- CMake-based project structure.
- Public C API declarations for all planned endpoints.
- Working project lifecycle API:
  - `mini_create_project`
  - `mini_open_project`
  - `mini_save_project`
  - `mini_close_project`
- Local project folder creation using the required directory layout.
- Basic CLI `create` command.
- Basic test for project create/save/open behavior.

## Not implemented yet

The remaining APIs currently return `MINI_ERROR_NOT_IMPLEMENTED` and will be delivered incrementally in later milestones:

- image import
- quality analysis
- segmentation/mask processing
- processed frame generation
- preview asset generation
- paint scheme and rendering
