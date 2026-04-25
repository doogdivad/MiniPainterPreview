package com.example.minipainter.ui.screens

import android.Manifest
import android.content.pm.PackageManager
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.camera.core.CameraSelector
import androidx.camera.core.ImageCapture
import androidx.camera.core.ImageCaptureException
import androidx.camera.core.Preview
import androidx.camera.lifecycle.ProcessCameraProvider
import androidx.camera.view.PreviewView
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Button
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.lifecycle.compose.LocalLifecycleOwner
import androidx.compose.ui.unit.dp
import androidx.compose.ui.viewinterop.AndroidView
import androidx.core.content.ContextCompat
import com.example.minipainter.data.CaptureMode
import com.example.minipainter.viewmodel.CaptureUiState
import java.io.File
import java.text.SimpleDateFormat
import java.util.Locale
import kotlinx.coroutines.launch

@Composable
fun CaptureScreen(
    projectId: String,
    projectPath: String,
    state: CaptureUiState,
    onModeChanged: (CaptureMode) -> Unit,
    onPhotoCaptured: (String, String) -> Unit,
    onFinished: () -> Unit
) {
    val context = LocalContext.current
    val lifecycleOwner = LocalLifecycleOwner.current
    val hasPermission = remember {
        mutableStateOf(
            ContextCompat.checkSelfPermission(context, Manifest.permission.CAMERA) ==
                PackageManager.PERMISSION_GRANTED
        )
    }

    val permissionLauncher = rememberLauncherForActivityResult(
        contract = ActivityResultContracts.RequestPermission()
    ) { granted ->
        hasPermission.value = granted
    }

    LaunchedEffect(Unit) {
        if (!hasPermission.value) permissionLauncher.launch(Manifest.permission.CAMERA)
    }

    if (!hasPermission.value) {
        Text("Camera permission required")
        return
    }

    val previewView = remember { PreviewView(context) }
    val imageCaptureState = remember {
        mutableStateOf(
            ImageCapture.Builder()
                .setCaptureMode(ImageCapture.CAPTURE_MODE_MAXIMIZE_QUALITY)
                .build()
        )
    }

    LaunchedEffect(previewView) {
        val provider = ProcessCameraProvider.getInstance(context).get()
        val preview = Preview.Builder().build().apply {
            surfaceProvider = previewView.surfaceProvider
        }
        provider.unbindAll()
        provider.bindToLifecycle(
            lifecycleOwner,
            CameraSelector.DEFAULT_BACK_CAMERA,
            preview,
            imageCaptureState.value
        )
    }

    val scope = rememberCoroutineScope()
    val ready = state as? CaptureUiState.Ready

    Column(modifier = Modifier.fillMaxSize().padding(12.dp), verticalArrangement = Arrangement.spacedBy(8.dp)) {
        Text("Project: $projectId")
        Row(horizontalArrangement = Arrangement.spacedBy(8.dp), modifier = Modifier.fillMaxWidth()) {
            CaptureMode.entries.forEach { mode ->
                Button(onClick = { onModeChanged(mode) }) { Text(mode.name.lowercase().replaceFirstChar { it.uppercase() }) }
            }
        }
        Text("Place the mini on a plain background")
        Text("Rotate the mini slightly between each photo")
        Text("Keep lighting consistent")
        Text("Progress: ${ready?.captured ?: 0} / ${ready?.mode?.totalPhotos ?: 0}")

        AndroidView(factory = { previewView }, modifier = Modifier.fillMaxWidth().height(360.dp))

        Button(
            enabled = ready?.processing == false,
            onClick = {
                val photoDir = File(projectPath, "raw").apply { mkdirs() }
                val photoFile = File(
                    photoDir,
                    "cam_${SimpleDateFormat("yyyyMMdd_HHmmss", Locale.US).format(System.currentTimeMillis())}.jpg"
                )
                val options = ImageCapture.OutputFileOptions.Builder(photoFile).build()
                imageCaptureState.value.takePicture(
                    options,
                    ContextCompat.getMainExecutor(context),
                    object : ImageCapture.OnImageSavedCallback {
                        override fun onImageSaved(outputFileResults: ImageCapture.OutputFileResults) {
                            scope.launch { onPhotoCaptured(projectPath, photoFile.absolutePath) }
                        }

                        override fun onError(exception: ImageCaptureException) {
                            // keep this as non-crashing placeholder for v1
                        }
                    }
                )
            }
        ) { Text("Capture") }

        Button(
            enabled = (ready?.captured ?: 0) >= (ready?.mode?.totalPhotos ?: 0),
            onClick = onFinished
        ) { Text("Finish") }

        ready?.warning?.let { Text("Warning: $it") }
    }
}
