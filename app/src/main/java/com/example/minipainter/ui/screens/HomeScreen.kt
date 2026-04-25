package com.example.minipainter.ui.screens

import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.FloatingActionButton
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.material3.TopAppBar
import androidx.compose.runtime.Composable
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import com.example.minipainter.data.MiniProjectSummary
import com.example.minipainter.viewmodel.HomeUiState

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun HomeScreen(
    state: HomeUiState,
    onNewMini: (String) -> Unit,
    onOpenProject: (MiniProjectSummary) -> Unit,
    onRefresh: () -> Unit
) {
    val showDialog = remember { mutableStateOf(false) }
    val nameState = remember { mutableStateOf("") }

    Scaffold(
        topBar = {
            TopAppBar(title = { Text("Mini Painter") })
        },
        floatingActionButton = {
            FloatingActionButton(onClick = { showDialog.value = true }) {
                Text("New Mini")
            }
        }
    ) { padding ->
        when (state) {
            HomeUiState.Loading -> Text("Loading...", modifier = Modifier.padding(padding).padding(16.dp))
            is HomeUiState.Error -> Column(modifier = Modifier.padding(padding).padding(16.dp)) {
                Text("Error: ${state.message}")
                TextButton(onClick = onRefresh) { Text("Retry") }
            }
            is HomeUiState.Success -> LazyColumn(
                modifier = Modifier.fillMaxSize().padding(padding),
                contentPadding = PaddingValues(16.dp),
                verticalArrangement = Arrangement.spacedBy(8.dp)
            ) {
                items(state.projects) { project ->
                    Column(
                        modifier = Modifier
                            .fillMaxWidth()
                            .clickable { onOpenProject(project) }
                            .padding(12.dp)
                    ) {
                        Text(project.name)
                        Text("captures: ${project.captureCount}")
                        Text("updated: ${project.updatedAt}")
                    }
                }
            }
        }
    }

    if (showDialog.value) {
        AlertDialog(
            onDismissRequest = { showDialog.value = false },
            title = { Text("Create Mini") },
            text = {
                OutlinedTextField(
                    value = nameState.value,
                    onValueChange = { nameState.value = it },
                    label = { Text("Mini name") }
                )
            },
            confirmButton = {
                TextButton(onClick = {
                    onNewMini(nameState.value.ifBlank { "Untitled Mini" })
                    showDialog.value = false
                    nameState.value = ""
                }) {
                    Text("Create")
                }
            },
            dismissButton = {
                TextButton(onClick = { showDialog.value = false }) { Text("Cancel") }
            }
        )
    }
}
