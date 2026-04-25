# Mini Painter (Android)

Mini Painter is a local-first Android frontend built with Kotlin + Jetpack Compose.

## Features in this scaffold

- Local project library in app-private storage (`filesDir/minis/`)
- Create/open projects from Home screen
- Guided CameraX capture flow with progress and quality warning surface
- JNI bridge to native backend via NDK + CMake
- Native **stub backend** for offline development when `mini_painter_core` is absent
- Preview builder + frame viewer (swipe/slider)
- Paint screen placeholder (local state only)

## Build

1. Open this folder in Android Studio (latest stable).
2. Let Gradle sync.
3. Run app on API 26+ device/emulator with camera support.

## Storage layout

Projects are stored under:

`context.filesDir/minis/{project_id}/`

Subfolders:

- `raw/`
- `masks/`
- `processed/`
- `preview/`
- `schemes/`

Metadata files:

- `project.json`
- `preview/preview.json`

## Native backend wiring

JNI bridge library name: `mini_painter_jni`

Current implementation is stubbed in:

- `app/src/main/cpp/mini_painter_jni.cpp`

To replace stub with real backend:

1. Put backend in `../mini_painter_core/`
2. Update `app/src/main/cpp/CMakeLists.txt` to `add_subdirectory` and link real target.
3. Keep exported JNI functions in `MiniPainterNative.kt` contract-compatible.

CMake receives `MINIPAINTER_CORE_DIR` from Gradle and will auto-detect/link common target names (`mini_painter_core`, `minipainter_core`, `mini_painter_core_static`) when available.

## Permissions

Only camera permission is requested:

- `android.permission.CAMERA`

No internet/login/subscription/cloud features are included.
