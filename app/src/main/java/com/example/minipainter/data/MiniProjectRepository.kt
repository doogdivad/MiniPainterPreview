package com.example.minipainter.data

import android.content.Context
import com.example.minipainter.nativebridge.MiniPainterNative
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import kotlinx.serialization.json.Json
import java.io.File

class MiniProjectRepository(private val context: Context) {
    private val json = Json { ignoreUnknownKeys = true }

    private val minisRoot: File
        get() = File(context.filesDir, "minis").apply { mkdirs() }

    suspend fun listProjects(): List<MiniProjectSummary> = withContext(Dispatchers.IO) {
        minisRoot.listFiles()
            ?.filter { it.isDirectory }
            ?.mapNotNull { projectDir ->
                val metadataFile = File(projectDir, "project.json")
                if (!metadataFile.exists()) return@mapNotNull null
                runCatching {
                    json.decodeFromString<MiniProjectSummary>(metadataFile.readText()).copy(
                        projectPath = projectDir.absolutePath
                    )
                }.getOrNull()
            }
            ?.sortedByDescending { it.updatedAt }
            .orEmpty()
    }

    suspend fun createProject(name: String): Result<MiniProjectSummary> = withContext(Dispatchers.IO) {
        runCatching {
            val nativeResult = MiniPainterNative.createProject(minisRoot.absolutePath, name)
            if (!nativeResult.ok || nativeResult.projectPath == null || nativeResult.projectId == null) {
                error(nativeResult.message ?: "Create project failed")
            }
            val metadataFile = File(nativeResult.projectPath, "project.json")
            if (!metadataFile.exists()) error("project.json missing")
            json.decodeFromString<MiniProjectSummary>(metadataFile.readText())
                .copy(projectPath = nativeResult.projectPath)
        }
    }

    suspend fun importCaptureImage(
        projectPath: String,
        imagePath: String,
        angleIndex: Int,
        totalPhotos: Int
    ): Result<NativeQualityReport> = withContext(Dispatchers.IO) {
        runCatching {
            val angle = angleIndex * 360.0 / totalPhotos
            val importResult = MiniPainterNative.importCaptureImage(projectPath, imagePath, angleIndex, angle)
            if (!importResult.ok) error(importResult.message ?: "Import failed")
            MiniPainterNative.analyseImageQuality(projectPath, "image_${angleIndex.toString().padStart(3, '0')}")
        }
    }

    suspend fun buildPreview(projectPath: String): Result<NativeResult> = withContext(Dispatchers.IO) {
        runCatching {
            val result = MiniPainterNative.buildPreviewAsset(projectPath)
            if (!result.ok) error(result.message ?: "Preview build failed")
            result
        }
    }

    suspend fun loadPreviewFrames(projectPath: String): List<String> = withContext(Dispatchers.IO) {
        val file = File(projectPath, "preview/preview.json")
        if (!file.exists()) return@withContext emptyList()
        json.decodeFromString<PreviewMetadata>(file.readText()).framePaths
    }
}
