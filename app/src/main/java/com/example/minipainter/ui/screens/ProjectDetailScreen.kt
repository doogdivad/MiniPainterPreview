package com.example.minipainter.ui.screens

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Button
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import com.example.minipainter.viewmodel.ProjectUiState

@Composable
fun ProjectDetailScreen(
    state: ProjectUiState,
    onContinueCapture: () -> Unit,
    onBuildPreview: () -> Unit,
    onViewPreview: () -> Unit,
    onPaint: () -> Unit
) {
    Column(
        modifier = Modifier.fillMaxSize().padding(16.dp),
        verticalArrangement = Arrangement.spacedBy(12.dp)
    ) {
        Text("Project Detail", style = MaterialTheme.typography.headlineMedium)
        Text(state.projectPath)
        Text("Frames: ${state.preview.frameCount}")
        Button(onClick = onContinueCapture) { Text("Continue Capture") }
        Button(onClick = onBuildPreview, enabled = !state.processing) {
            Text(if (state.processing) "Building..." else "Build Preview")
        }
        Button(onClick = onViewPreview, enabled = state.preview.frameCount > 0) { Text("View Preview") }
        Button(onClick = onPaint) { Text("Paint") }
        state.result?.let { Text("Build result: ${it.code} ${it.message.orEmpty()}") }
        state.error?.let { Text("Error: $it") }
    }
}
