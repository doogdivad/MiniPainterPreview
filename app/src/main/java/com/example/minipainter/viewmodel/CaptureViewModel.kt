package com.example.minipainter.viewmodel

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.example.minipainter.data.CaptureMode
import com.example.minipainter.data.MiniProjectRepository
import com.example.minipainter.data.NativeQualityReport
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch

sealed interface CaptureUiState {
    data class Ready(
        val captured: Int = 0,
        val mode: CaptureMode = CaptureMode.BETTER,
        val lastQuality: NativeQualityReport? = null,
        val warning: String? = null,
        val processing: Boolean = false
    ) : CaptureUiState

    data class Error(val message: String) : CaptureUiState
}

class CaptureViewModel(private val repository: MiniProjectRepository) : ViewModel() {
    private val _uiState = MutableStateFlow<CaptureUiState>(CaptureUiState.Ready())
    val uiState: StateFlow<CaptureUiState> = _uiState.asStateFlow()

    fun setMode(mode: CaptureMode) {
        val current = _uiState.value
        if (current is CaptureUiState.Ready) _uiState.value = current.copy(mode = mode)
    }

    fun onCaptured(projectPath: String, imagePath: String) {
        val current = _uiState.value as? CaptureUiState.Ready ?: return
        viewModelScope.launch {
            _uiState.value = current.copy(processing = true)
            repository.importCaptureImage(
                projectPath = projectPath,
                imagePath = imagePath,
                angleIndex = current.captured,
                totalPhotos = current.mode.totalPhotos
            ).onSuccess { quality ->
                val nextCount = current.captured + 1
                _uiState.value = current.copy(
                    captured = nextCount,
                    lastQuality = quality,
                    warning = quality.warnings.firstOrNull(),
                    processing = false
                )
            }.onFailure {
                _uiState.value = CaptureUiState.Error(it.message ?: "Capture import failed")
            }
        }
    }
}
