package com.example.minipainter.viewmodel

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.example.minipainter.data.MiniProjectRepository
import com.example.minipainter.data.MiniProjectSummary
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

data class HomeUiState(
    val loading: Boolean = false,
    val projects: List<MiniProjectSummary> = emptyList(),
    val error: String? = null
)

class HomeViewModel(private val repository: MiniProjectRepository) : ViewModel() {
    private val _uiState = MutableStateFlow(HomeUiState(loading = true))
    val uiState: StateFlow<HomeUiState> = _uiState.asStateFlow()

    init {
        refresh()
    }

    fun refresh() {
        viewModelScope.launch {
            _uiState.value = _uiState.value.copy(loading = true, error = null)
            runCatching {
                withContext(Dispatchers.IO) { repository.listProjects() }
            }.onSuccess { projects ->
                _uiState.value = HomeUiState(loading = false, projects = projects)
            }.onFailure {
                _uiState.value = HomeUiState(loading = false, error = it.message)
            }
        }
    }

    suspend fun createProject(name: String): String? = withContext(Dispatchers.IO) {
        val result = repository.createProject(name)
        refresh()
        result.projectPath
    }
}
