package com.example.minipainter.viewmodel

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.example.minipainter.data.MiniProjectRepository
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch

sealed interface ProjectUiState {
    data object Idle : ProjectUiState
    data object Processing : ProjectUiState
    data class Success(val message: String) : ProjectUiState
    data class Error(val message: String) : ProjectUiState
}

class ProjectViewModel(private val repository: MiniProjectRepository) : ViewModel() {
    private val _uiState = MutableStateFlow<ProjectUiState>(ProjectUiState.Idle)
    val uiState: StateFlow<ProjectUiState> = _uiState.asStateFlow()

    fun buildPreview(projectPath: String) {
        viewModelScope.launch {
            _uiState.value = ProjectUiState.Processing
            repository.buildPreview(projectPath)
                .onSuccess {
                    _uiState.value = ProjectUiState.Success("Preview build complete")
                }
                .onFailure {
                    _uiState.value = ProjectUiState.Error(it.message ?: "Preview build failed")
                }
        }
    }

    fun resetStatus() {
        _uiState.value = ProjectUiState.Idle
    }

    suspend fun loadPreviewFrames(projectPath: String): List<String> =
        repository.loadPreviewFrames(projectPath)
}
