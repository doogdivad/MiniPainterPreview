package com.example.minipainter

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import com.example.minipainter.data.MiniProjectRepository
import com.example.minipainter.navigation.MiniPainterNavGraph
import com.example.minipainter.viewmodel.MiniPainterViewModelFactory

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        val repository = MiniProjectRepository(applicationContext)
        val factory = MiniPainterViewModelFactory(repository)
        setContent {
            MiniPainterNavGraph(factory = factory)
        }
    }
}
