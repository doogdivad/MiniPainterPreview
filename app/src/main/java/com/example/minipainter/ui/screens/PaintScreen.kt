package com.example.minipainter.ui.screens

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Button
import androidx.compose.material3.Slider
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp

@Composable
fun PaintScreen(projectPath: String) {
    var red by remember { mutableFloatStateOf(0.6f) }
    var green by remember { mutableFloatStateOf(0.2f) }
    var blue by remember { mutableFloatStateOf(0.2f) }
    var opacity by remember { mutableFloatStateOf(1f) }
    val status = remember { mutableStateOf("paint rendering not implemented yet") }

    val color = Color(red, green, blue, opacity)

    Column(
        modifier = Modifier.fillMaxSize().padding(16.dp),
        verticalArrangement = Arrangement.spacedBy(12.dp)
    ) {
        Text("Paint Scheme (placeholder)")
        Text("Project path: $projectPath")
        Row(modifier = Modifier.fillMaxWidth().height(120.dp).background(color)) {}

        Text("Red")
        Slider(value = red, onValueChange = { red = it })
        Text("Green")
        Slider(value = green, onValueChange = { green = it })
        Text("Blue")
        Slider(value = blue, onValueChange = { blue = it })
        Text("Opacity")
        Slider(value = opacity, onValueChange = { opacity = it })

        Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
            Button(onClick = { status.value = "Layer added (local placeholder)" }) { Text("Add Layer") }
            Button(onClick = { status.value = "Scheme saved locally (placeholder)" }) { Text("Save Scheme") }
            Button(onClick = { status.value = "paint rendering not implemented yet" }) { Text("Render Preview") }
        }
        Text(status.value)
    }
}
