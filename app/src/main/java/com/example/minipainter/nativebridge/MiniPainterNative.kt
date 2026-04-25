package com.example.minipainter.nativebridge

import com.example.minipainter.data.NativeProjectResult
import com.example.minipainter.data.NativeQualityReport
import com.example.minipainter.data.NativeResult
import kotlinx.serialization.json.Json

/**
 * JNI boundary stays isolated here so repository/view models can work with Kotlin models.
 */
object MiniPainterNative {
    private val json = Json { ignoreUnknownKeys = true }

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

    fun createProject(rootDir: String, displayName: String): NativeProjectResult =
        json.decodeFromString(createProjectJson(rootDir, displayName))

    fun openProject(projectPath: String): NativeProjectResult =
        json.decodeFromString(openProjectJson(projectPath))

    fun importCaptureImage(
        projectPath: String,
        imagePath: String,
        angleIndex: Int,
        estimatedAngleDegrees: Double
    ): NativeResult = json.decodeFromString(
        importCaptureImageJson(projectPath, imagePath, angleIndex, estimatedAngleDegrees)
    )

    fun analyseImageQuality(projectPath: String, imageId: String): NativeQualityReport =
        json.decodeFromString(analyseImageQualityJson(projectPath, imageId))

    fun buildPreviewAsset(projectPath: String): NativeResult =
        json.decodeFromString(buildPreviewAssetJson(projectPath))

    fun renderSchemeFrame(
        projectPath: String,
        schemeId: String,
        frameIndex: Int,
        outputPath: String
    ): NativeResult = json.decodeFromString(
        renderSchemeFrameJson(projectPath, schemeId, frameIndex, outputPath)
    )
}
