package com.example.minipainter.viewmodel

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.example.minipainter.data.MiniProjectRepository
import com.example.minipainter.data.MiniProjectSummary
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch

sealed interface HomeUiState {
    data object Loading : HomeUiState
    data class Success(val projects: List<MiniProjectSummary>) : HomeUiState
    data class Error(val message: String) : HomeUiState
}

class HomeViewModel(private val repository: MiniProjectRepository) : ViewModel() {
    private val _uiState = MutableStateFlow<HomeUiState>(HomeUiState.Loading)
    val uiState: StateFlow<HomeUiState> = _uiState.asStateFlow()

    init {
        refresh()
    }

    fun refresh() {
        viewModelScope.launch {
            _uiState.value = HomeUiState.Loading
            _uiState.value = runCatching { repository.listProjects() }
                .fold(
                    onSuccess = { HomeUiState.Success(it) },
                    onFailure = { HomeUiState.Error(it.message ?: "Unable to load projects") }
                )
        }
    }

    fun createProject(name: String, onCreated: (MiniProjectSummary) -> Unit) {
        viewModelScope.launch {
            repository.createProject(name)
                .onSuccess {
                    onCreated(it)
                    refresh()
                }
                .onFailure {
                    _uiState.value = HomeUiState.Error(it.message ?: "Create failed")
                }
        }
    }
}
