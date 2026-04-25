package com.example.minipainter.data

import kotlinx.serialization.Serializable

@Serializable
data class MiniProjectSummary(
    val projectId: String,
    val name: String,
    val projectPath: String,
    val thumbnailPath: String? = null,
    val captureCount: Int = 0,
    val createdAt: Long,
    val updatedAt: Long
)

@Serializable
data class NativeResult(
    val ok: Boolean,
    val code: String,
    val message: String? = null
)

@Serializable
data class NativeProjectResult(
    val ok: Boolean,
    val code: String,
    val message: String? = null,
    val projectId: String? = null,
    val projectPath: String? = null
)

@Serializable
data class NativeQualityReport(
    val ok: Boolean,
    val blurScore: Double,
    val exposureScore: Double,
    val warnings: List<String> = emptyList()
)

@Serializable
data class PreviewMetadata(
    val framePaths: List<String> = emptyList()
)

enum class CaptureMode(val totalPhotos: Int) {
    QUICK(12),
    BETTER(24),
    STUDIO(36)
}
