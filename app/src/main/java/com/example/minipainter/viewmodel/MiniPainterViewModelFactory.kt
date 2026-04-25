package com.example.minipainter.viewmodel

import androidx.lifecycle.ViewModel
import androidx.lifecycle.ViewModelProvider
import com.example.minipainter.data.MiniProjectRepository

class MiniPainterViewModelFactory(
    private val repository: MiniProjectRepository
) : ViewModelProvider.Factory {
    override fun <T : ViewModel> create(modelClass: Class<T>): T {
        return when {
            modelClass.isAssignableFrom(HomeViewModel::class.java) -> HomeViewModel(repository) as T
            modelClass.isAssignableFrom(CaptureViewModel::class.java) -> CaptureViewModel(repository) as T
            modelClass.isAssignableFrom(ProjectViewModel::class.java) -> ProjectViewModel(repository) as T
            else -> error("Unknown viewmodel: ${modelClass.name}")
        }
    }
}
