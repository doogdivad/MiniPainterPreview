package com.example.minipainter.ui.screens

import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Add
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Card
import androidx.compose.material3.FloatingActionButton
import androidx.compose.material3.Icon
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.material3.TextField
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import com.example.minipainter.viewmodel.HomeUiState
import java.text.DateFormat
import java.util.Date

@Composable
fun HomeScreen(
    state: HomeUiState,
    onRefresh: () -> Unit,
    onCreateProject: (String) -> Unit,
    onOpenProject: (String) -> Unit
) {
    var showCreateDialog by remember { mutableStateOf(false) }
    var projectName by remember { mutableStateOf("") }

    Scaffold(
        floatingActionButton = {
            FloatingActionButton(onClick = { showCreateDialog = true }) {
                Icon(Icons.Default.Add, contentDescription = "New Mini")
            }
        }
    ) { padding ->
        Column(modifier = Modifier.fillMaxSize().padding(padding).padding(16.dp)) {
            Text("Mini Painter", style = MaterialTheme.typography.headlineMedium)
            TextButton(onClick = onRefresh) { Text("Refresh") }
            if (state.error != null) Text("Error: ${state.error}")
            LazyColumn(verticalArrangement = Arrangement.spacedBy(8.dp)) {
                items(state.projects, key = { it.projectId }) { project ->
                    Card(modifier = Modifier.fillMaxWidth().clickable { onOpenProject(project.projectPath) }) {
                        Row(modifier = Modifier.fillMaxWidth().padding(12.dp)) {
                            Column {
                                Text(project.name, style = MaterialTheme.typography.titleMedium)
                                Text("Captures: ${project.captureCount}")
                                Text("Updated: ${DateFormat.getDateTimeInstance().format(Date(project.updatedAt))}")
                            }
                        }
                    }
                }
            }
        }
    }

    if (showCreateDialog) {
        AlertDialog(
            onDismissRequest = { showCreateDialog = false },
            confirmButton = {
                TextButton(
                    onClick = {
                        onCreateProject(projectName)
                        showCreateDialog = false
                        projectName = ""
                    },
                    enabled = projectName.isNotBlank()
                ) { Text("Create") }
            },
            dismissButton = { TextButton(onClick = { showCreateDialog = false }) { Text("Cancel") } },
            title = { Text("Create New Mini") },
            text = {
                TextField(value = projectName, onValueChange = { projectName = it }, label = { Text("Mini name") })
            }
        )
    }
}
