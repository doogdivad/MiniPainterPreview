package com.example.minipainter.viewmodel

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.example.minipainter.data.MiniProjectRepository
import com.example.minipainter.data.NativeResult
import com.example.minipainter.data.PreviewMetadata
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

data class ProjectUiState(
    val loading: Boolean = false,
    val processing: Boolean = false,
    val projectPath: String = "",
    val preview: PreviewMetadata = PreviewMetadata(0, emptyList()),
    val result: NativeResult? = null,
    val error: String? = null
)

class ProjectViewModel(private val repository: MiniProjectRepository) : ViewModel() {
    private val _uiState = MutableStateFlow(ProjectUiState())
    val uiState: StateFlow<ProjectUiState> = _uiState.asStateFlow()

    fun load(projectPath: String) {
        viewModelScope.launch {
            _uiState.value = _uiState.value.copy(loading = true, projectPath = projectPath)
            runCatching {
                withContext(Dispatchers.IO) { repository.loadPreview(projectPath) }
            }.onSuccess { preview ->
                _uiState.value = _uiState.value.copy(loading = false, preview = preview)
            }.onFailure {
                _uiState.value = _uiState.value.copy(loading = false, error = it.message)
            }
        }
    }

    fun buildPreview() {
        val path = _uiState.value.projectPath
        if (path.isBlank()) return
        viewModelScope.launch {
            _uiState.value = _uiState.value.copy(processing = true, error = null)
            runCatching {
                withContext(Dispatchers.IO) {
                    val result = repository.buildPreview(path)
                    val preview = repository.loadPreview(path)
                    result to preview
                }
            }.onSuccess { (result, preview) ->
                _uiState.value = _uiState.value.copy(processing = false, result = result, preview = preview)
            }.onFailure {
                _uiState.value = _uiState.value.copy(processing = false, error = it.message)
            }
        }
    }
}
