package com.example.minipainter.ui.screens

import androidx.compose.foundation.Image
import androidx.compose.foundation.gestures.detectHorizontalDragGestures
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Button
import androidx.compose.material3.Slider
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.unit.dp
import coil.compose.rememberAsyncImagePainter
import com.example.minipainter.data.MiniProjectRepository

@Composable
fun PreviewScreen(
    projectPath: String,
    repository: MiniProjectRepository,
    onPaint: (String) -> Unit
) {
    val frames = remember { mutableStateOf<List<String>>(emptyList()) }
    var frameIndex by remember { mutableFloatStateOf(0f) }

    LaunchedEffect(projectPath) {
        frames.value = repository.loadPreviewFrames(projectPath)
        frameIndex = 0f
    }

    val currentIndex = frameIndex.toInt().coerceIn(0, (frames.value.size - 1).coerceAtLeast(0))

    Column(
        modifier = Modifier.fillMaxSize().padding(16.dp),
        verticalArrangement = Arrangement.spacedBy(8.dp)
    ) {
        Text("Preview")
        if (frames.value.isEmpty()) {
            Text("No preview frames found")
        } else {
            Image(
                painter = rememberAsyncImagePainter(model = frames.value[currentIndex]),
                contentDescription = "Mini preview frame",
                contentScale = ContentScale.Fit,
                modifier = Modifier
                    .weight(1f)
                    .pointerInput(frames.value.size) {
                        detectHorizontalDragGestures { _, dragAmount ->
                            frameIndex = (frameIndex - (dragAmount / 25f)).coerceIn(0f, (frames.value.size - 1).toFloat())
                        }
                    }
            )
            Slider(
                value = frameIndex,
                onValueChange = { frameIndex = it },
                valueRange = 0f..(frames.value.size - 1).toFloat()
            )
            Text("Frame ${currentIndex + 1} / ${frames.value.size}")
        }
        Button(onClick = { onPaint(projectPath) }) { Text("Paint") }
    }
}
