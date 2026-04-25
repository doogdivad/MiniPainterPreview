package com.example.minipainter.ui.screens

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Button
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Slider
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp

@Composable
fun PaintScreen(projectPath: String) {
    var colorIndex by remember { mutableIntStateOf(0) }
    var opacity by remember { mutableFloatStateOf(1f) }
    val colors = listOf(Color.Red, Color.Green, Color.Blue, Color.Yellow)

    Column(
        modifier = Modifier.fillMaxSize().padding(16.dp),
        verticalArrangement = Arrangement.spacedBy(10.dp)
    ) {
        Text("Paint", style = MaterialTheme.typography.headlineMedium)
        Text("Project: $projectPath")
        Box(
            modifier = Modifier
                .fillMaxWidth()
                .height(240.dp)
                .background(colors[colorIndex].copy(alpha = opacity))
        )
        Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
            colors.forEachIndexed { idx, _ ->
                Button(onClick = { colorIndex = idx }) { Text("C${idx + 1}") }
            }
        }
        Text("Opacity")
        Slider(value = opacity, onValueChange = { opacity = it })
        Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
            Button(onClick = { }) { Text("Add Layer") }
            Button(onClick = { }) { Text("Save Scheme") }
            Button(onClick = { }) { Text("Render Preview") }
        }
        Text("paint rendering not implemented yet")
    }
}
