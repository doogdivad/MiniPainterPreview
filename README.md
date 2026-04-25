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
