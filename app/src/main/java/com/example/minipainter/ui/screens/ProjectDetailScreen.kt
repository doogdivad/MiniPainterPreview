package com.example.minipainter.ui.screens

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Button
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import com.example.minipainter.viewmodel.ProjectUiState

@Composable
fun ProjectDetailScreen(
    projectId: String,
    name: String,
    projectPath: String,
    captureCount: Int,
    state: ProjectUiState,
    onContinueCapture: (String, String) -> Unit,
    onBuildPreview: (String) -> Unit,
    onViewPreview: (String) -> Unit,
    onPaint: (String) -> Unit
) {
    Column(
        modifier = Modifier.fillMaxSize().padding(16.dp),
        verticalArrangement = Arrangement.spacedBy(8.dp)
    ) {
        Text("Project: $name")
        Text("Capture count: $captureCount")
        Button(onClick = { onContinueCapture(projectPath, projectId) }) { Text("Continue Capture") }
        Button(onClick = { onBuildPreview(projectPath) }) { Text("Build Preview") }
        Button(onClick = { onViewPreview(projectPath) }) { Text("View Preview") }
        Button(onClick = { onPaint(projectPath) }) { Text("Paint") }

        when (state) {
            ProjectUiState.Idle -> Unit
            ProjectUiState.Processing -> Text("Building preview...")
            is ProjectUiState.Success -> Text(state.message)
            is ProjectUiState.Error -> Text("Error: ${state.message}")
        }
    }
}
