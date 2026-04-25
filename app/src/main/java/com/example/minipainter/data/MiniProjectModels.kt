package com.example.minipainter.data

import kotlinx.serialization.SerialName
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
    val frameCount: Int,
    val frames: List<String>
)

@Serializable
data class ProjectDiskModel(
    @SerialName("project_id") val projectId: String,
    @SerialName("display_name") val displayName: String,
    @SerialName("created_at") val createdAt: Long,
    @SerialName("updated_at") val updatedAt: Long,
    @SerialName("capture_count") val captureCount: Int = 0,
    @SerialName("thumbnail_path") val thumbnailPath: String? = null
)
