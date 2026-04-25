package com.example.minipainter.data

import android.content.Context
import com.example.minipainter.native.MiniPainterNative
import kotlinx.serialization.SerializationException
import kotlinx.serialization.json.Json
import java.io.File

class MiniProjectRepository(private val context: Context) {
    private val json = Json { ignoreUnknownKeys = true }

    val minisRoot: File = File(context.filesDir, "minis").also { it.mkdirs() }

    fun listProjects(): List<MiniProjectSummary> {
        if (!minisRoot.exists()) return emptyList()
        return minisRoot.listFiles()
            .orEmpty()
            .filter(File::isDirectory)
            .mapNotNull { folder ->
                val model = readProjectDiskModel(folder) ?: return@mapNotNull null
                MiniProjectSummary(
                    projectId = model.projectId,
                    name = model.displayName,
                    projectPath = folder.absolutePath,
                    thumbnailPath = model.thumbnailPath,
                    captureCount = model.captureCount,
                    createdAt = model.createdAt,
                    updatedAt = model.updatedAt
                )
            }
            .sortedByDescending { it.updatedAt }
    }

    fun createProject(displayName: String): NativeProjectResult {
        val raw = MiniPainterNative.createProjectJson(minisRoot.absolutePath, displayName)
        return parse(raw)
    }

    fun importCaptureImage(
        projectPath: String,
        imagePath: String,
        angleIndex: Int,
        estimatedAngleDegrees: Double
    ): NativeResult {
        val raw = MiniPainterNative.importCaptureImageJson(
            projectPath,
            imagePath,
            angleIndex,
            estimatedAngleDegrees
        )
        return parse(raw)
    }

    fun analyseImageQuality(projectPath: String, imageId: String): NativeQualityReport {
        val raw = MiniPainterNative.analyseImageQualityJson(projectPath, imageId)
        return parse(raw)
    }

    fun buildPreview(projectPath: String): NativeResult {
        val raw = MiniPainterNative.buildPreviewAssetJson(projectPath)
        return parse(raw)
    }

    fun loadPreview(projectPath: String): PreviewMetadata {
        val file = File(File(projectPath, "preview"), "preview.json")
        if (!file.exists()) return PreviewMetadata(0, emptyList())
        return json.decodeFromString(file.readText())
    }

    private fun readProjectDiskModel(folder: File): ProjectDiskModel? {
        val file = File(folder, "project.json")
        if (!file.exists()) return null
        return try {
            json.decodeFromString<ProjectDiskModel>(file.readText())
        } catch (_: SerializationException) {
            null
        }
    }

    private inline fun <reified T> parse(payload: String): T {
        return json.decodeFromString(payload)
    }
}
