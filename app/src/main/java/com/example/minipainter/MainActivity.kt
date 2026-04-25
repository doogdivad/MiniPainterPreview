package com.example.minipainter

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import com.example.minipainter.data.MiniProjectRepository
import com.example.minipainter.navigation.MiniPainterNavHost

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        val repository = MiniProjectRepository(applicationContext)
        setContent {
            MaterialTheme {
                Surface {
                    MiniPainterNavHost(repository = repository)
                }
            }
        }
    }
}
