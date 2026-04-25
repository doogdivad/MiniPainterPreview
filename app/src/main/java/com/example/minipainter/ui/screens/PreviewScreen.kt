package com.example.minipainter.ui.screens

import androidx.compose.foundation.Image
import androidx.compose.foundation.gestures.detectHorizontalDragGestures
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Button
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Slider
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.unit.dp
import coil.compose.rememberAsyncImagePainter
import com.example.minipainter.data.PreviewMetadata

@Composable
fun PreviewScreen(preview: PreviewMetadata, onPaint: () -> Unit) {
    var index by remember { mutableIntStateOf(0) }
    val max = (preview.frameCount - 1).coerceAtLeast(0)
    val currentFrame = preview.frames.getOrNull(index)

    Column(
        modifier = Modifier.fillMaxSize().padding(16.dp),
        verticalArrangement = Arrangement.spacedBy(12.dp)
    ) {
        Text("Preview", style = MaterialTheme.typography.headlineMedium)
        Text("Frame ${index + 1} / ${preview.frameCount}")
        Image(
            painter = rememberAsyncImagePainter(model = currentFrame),
            contentDescription = null,
            modifier = Modifier
                .fillMaxWidth()
                .weight(1f)
                .pointerInput(max) {
                    detectHorizontalDragGestures { _, dragAmount ->
                        if (dragAmount > 0) index = (index - 1).coerceAtLeast(0)
                        else index = (index + 1).coerceAtMost(max)
                    }
                }
        )
        Slider(
            value = index.toFloat(),
            onValueChange = { index = it.toInt().coerceIn(0, max) },
            valueRange = 0f..max.toFloat().coerceAtLeast(0f)
        )
        Button(onClick = onPaint) { Text("Paint") }
    }
}
