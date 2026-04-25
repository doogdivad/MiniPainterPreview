# Mini Painter Core (Milestones 1-2)

This repository contains a portable C++ core library scaffold for Mini Painter.

## Build

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Targets

- `mini_painter_core`: static library exposing a stable C API wrapper.
- `mini_painter_cli`: minimal CLI tool (`create`, `import`).
- `mini_painter_tests`: executable test for project lifecycle and image import metadata persistence.

## Implemented so far

- CMake-based project structure.
- Public C API declarations for all planned endpoints.
- Working project lifecycle API:
  - `mini_create_project`
  - `mini_open_project`
  - `mini_save_project`
  - `mini_close_project`
- Local project folder creation using the required directory layout.
- Basic CLI `create` command.
- Image import API (`mini_import_capture_image`) with:
  - JPEG/PNG extension validation
  - copy into `raw/` as `image_###.<ext>`
  - generated `image_id`
  - metadata persistence in `project.json` `capture_set`
  - no overwrite of existing destination files
- CLI `import` command.
- Tests for project create/save/open plus import/reload metadata behavior.

## Not implemented yet

The remaining APIs currently return `MINI_ERROR_NOT_IMPLEMENTED` and will be delivered incrementally in later milestones:

- quality analysis
- segmentation/mask processing
- processed frame generation
- preview asset generation
- paint scheme and rendering
