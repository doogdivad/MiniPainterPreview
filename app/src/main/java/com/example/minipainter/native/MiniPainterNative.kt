package com.example.minipainter.native

/**
 * JNI boundary for Mini Painter. JSON payloads keep v1 integration simple and stable.
 */
object MiniPainterNative {
    init {
        System.loadLibrary("mini_painter_jni")
    }

    external fun createProjectJson(rootDir: String, displayName: String): String
    external fun openProjectJson(projectPath: String): String
    external fun importCaptureImageJson(
        projectPath: String,
        imagePath: String,
        angleIndex: Int,
        estimatedAngleDegrees: Double
    ): String

    external fun analyseImageQualityJson(projectPath: String, imageId: String): String
    external fun buildPreviewAssetJson(projectPath: String): String
    external fun renderSchemeFrameJson(
        projectPath: String,
        schemeId: String,
        frameIndex: Int,
        outputPath: String
    ): String
}
