# Mini Painter Android Frontend (Stubbed Native Backend)

This repository now contains a Kotlin Android app prototype for **Mini Painter** plus a JNI C++ stub backend so frontend work can proceed before the full native core is integrated.

## What is included

- Jetpack Compose app shell with AndroidX Navigation.
- Local-first project library stored in app-private storage (`files/minis`).
- CameraX capture flow with quality capture mode.
- JNI bridge (`MiniPainterNative`) returning JSON payloads parsed in Kotlin.
- NDK + CMake native module (`mini_painter_jni`) with stubbed behavior:
  - creates project folders and `project.json`
  - imports capture images to `raw/`
  - returns fake quality reports
  - builds a fake preview manifest by reusing imported images
- Screens:
  - Home
  - Project Detail
  - Capture
  - Preview
  - Paint (placeholder)

## Android build prerequisites

- Android Studio (recent stable)
- Android SDK with API 35 (or adjust in `app/build.gradle.kts`)
- Android NDK + CMake installed from SDK Manager

## Build and run

1. Open this repository in Android Studio.
2. Let Gradle sync.
3. Select an emulator/device (API 26+).
4. Run the `app` module.

## Storage layout

The app writes all data to app-private storage:

```text
<context.filesDir>/minis/{project_id}/
  project.json
  raw/
  masks/
  processed/
  preview/
  schemes/
```

No broad storage permissions are requested.

## Native integration and stub mode

The JNI layer is built from:

- `app/src/main/cpp/CMakeLists.txt`
- `app/src/main/cpp/mini_painter_jni.cpp`

The current implementation is a stub. Later, replace it by linking the real C++ core with CMake `add_subdirectory(...)` and swapping stub logic for production calls.

## Existing C++ core scaffold

The original C++ library scaffold remains in the repository root for iterative native development and tests.

## Native core progress (milestones)

- ✅ Milestone 1: skeleton library/CLI/tests + project create/open/save.
- ✅ Milestone 2: JPEG/PNG import into `raw/` with persisted capture metadata.
- ✅ Milestone 3: image quality analysis is now implemented in `mini_painter_core`:
  - OpenCV-based decode + metrics (blur, exposure, dimensions, subject coverage estimate)
  - warning flags (`too blurry`, `too dark`, `too bright`, `likely cropped`, `low resolution`)
  - persisted per-image `quality_score`
  - CLI support via:
    - `mini_painter_cli analyse --project <project_dir> --image-id <id>`
  - tests now cover valid-image quality analysis and corrupt-image decode handling.

> Note: when OpenCV is not available in the build environment, the project still builds, but
> `mini_analyse_image_quality` returns `MINI_ERROR_NOT_IMPLEMENTED`.

- ✅ Milestone 4: mask handling is now implemented in `mini_painter_core`:
  - external mask import via `mini_set_external_mask`
  - fallback threshold-based background segmentation when no mask exists
  - mask cleanup pipeline via `mini_cleanup_mask`:
    - remove small islands (connected component size filter)
    - fill holes
    - optional feather + dilate/erode operations
  - mask metadata persisted into `project.json` under each capture image
  - CLI support via:
    - `mini_painter_cli set-mask --project <project_dir> --image-id <id> --mask <path>`
    - `mini_painter_cli cleanup-mask --project <project_dir> --image-id <id> [--remove-islands <px>] [--fill-holes <0|1>] [--feather <px>] [--dilate-erode <-n..n>]`
  - tests now validate mask write/import and cleanup behavior.
