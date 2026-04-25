package com.example.minipainter.viewmodel

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.example.minipainter.data.MiniProjectRepository
import com.example.minipainter.data.NativeQualityReport
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import java.io.File

enum class CaptureMode(val totalPhotos: Int) {
    QUICK(12),
    BETTER(24),
    STUDIO(36)
}

data class CaptureUiState(
    val processing: Boolean = false,
    val projectPath: String = "",
    val mode: CaptureMode = CaptureMode.BETTER,
    val currentCount: Int = 0,
    val qualityReport: NativeQualityReport? = null,
    val error: String? = null
)

class CaptureViewModel(private val repository: MiniProjectRepository) : ViewModel() {
    private val _uiState = MutableStateFlow(CaptureUiState())
    val uiState: StateFlow<CaptureUiState> = _uiState.asStateFlow()

    fun loadProject(projectPath: String, mode: CaptureMode) {
        _uiState.value = CaptureUiState(projectPath = projectPath, mode = mode)
    }

    fun onPhotoCaptured(file: File) {
        val state = _uiState.value
        if (state.projectPath.isBlank()) return
        val nextIndex = state.currentCount
        val angle = nextIndex * 360.0 / state.mode.totalPhotos.toDouble()
        viewModelScope.launch {
            _uiState.value = state.copy(processing = true, error = null)
            runCatching {
                withContext(Dispatchers.IO) {
                    repository.importCaptureImage(state.projectPath, file.absolutePath, nextIndex, angle)
                    repository.analyseImageQuality(state.projectPath, file.name)
                }
            }.onSuccess { report ->
                _uiState.value = _uiState.value.copy(
                    processing = false,
                    currentCount = nextIndex + 1,
                    qualityReport = report,
                    error = null
                )
            }.onFailure {
                _uiState.value = _uiState.value.copy(processing = false, error = it.message)
            }
        }
    }
}
