package com.example.minipainter.ui.screens

import android.annotation.SuppressLint
import android.content.Context
import androidx.camera.core.CameraSelector
import androidx.camera.core.ImageCapture
import androidx.camera.core.ImageCaptureException
import androidx.camera.core.Preview
import androidx.camera.lifecycle.ProcessCameraProvider
import androidx.camera.view.PreviewView
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Button
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.platform.LocalLifecycleOwner
import androidx.compose.ui.unit.dp
import androidx.compose.ui.viewinterop.AndroidView
import androidx.core.content.ContextCompat
import com.example.minipainter.viewmodel.CaptureUiState
import java.io.File
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale
import java.util.concurrent.ExecutorService
import java.util.concurrent.Executors

@SuppressLint("RestrictedApi")
@Composable
fun CaptureScreen(
    state: CaptureUiState,
    onPhotoCaptured: (File) -> Unit,
    onFinish: () -> Unit
) {
    val context = LocalContext.current
    val lifecycleOwner = LocalLifecycleOwner.current
    val executor = remember { Executors.newSingleThreadExecutor() }
    val imageCaptureState = remember { mutableStateOf<ImageCapture?>(null) }

    LaunchedEffect(state.projectPath) {
        if (state.projectPath.isNotBlank()) {
            bindCamera(context, lifecycleOwner, imageCaptureState.value) { capture ->
                imageCaptureState.value = capture
            }
        }
    }

    Column(modifier = Modifier.fillMaxSize().padding(16.dp), verticalArrangement = Arrangement.spacedBy(12.dp)) {
        Text("Capture", style = MaterialTheme.typography.headlineMedium)
        Text("Photo ${state.currentCount} / ${state.mode.totalPhotos}")
        Text("Place the mini on a plain background")
        Text("Rotate the mini slightly between each photo")
        Text("Keep lighting consistent")
        AndroidView(
            modifier = Modifier.fillMaxWidth().weight(1f),
            factory = { PreviewView(it) }
        ) { previewView ->
            bindCamera(context, lifecycleOwner, previewView) { capture ->
                imageCaptureState.value = capture
            }
        }
        state.qualityReport?.warnings?.takeIf { it.isNotEmpty() }?.let {
            Text("Quality warning: ${it.joinToString()}")
        }
        Button(onClick = {
            capturePhoto(context, state.projectPath, imageCaptureState.value, executor) { file ->
                onPhotoCaptured(file)
            }
        }) {
            Text(if (state.processing) "Processing..." else "Capture")
        }
        Button(onClick = onFinish, enabled = state.currentCount >= state.mode.totalPhotos / 2) {
            Text("Finish")
        }
        state.error?.let { Text("Error: $it") }
    }
}

private fun bindCamera(
    context: Context,
    lifecycleOwner: androidx.lifecycle.LifecycleOwner,
    previewView: PreviewView,
    onCaptureReady: (ImageCapture) -> Unit
) {
    val cameraProviderFuture = ProcessCameraProvider.getInstance(context)
    cameraProviderFuture.addListener({
        val cameraProvider = cameraProviderFuture.get()
        val preview = Preview.Builder().build().also { it.surfaceProvider = previewView.surfaceProvider }
        val imageCapture = ImageCapture.Builder()
            .setCaptureMode(ImageCapture.CAPTURE_MODE_MAXIMIZE_QUALITY)
            .build()
        cameraProvider.unbindAll()
        cameraProvider.bindToLifecycle(
            lifecycleOwner,
            CameraSelector.DEFAULT_BACK_CAMERA,
            preview,
            imageCapture
        )
        onCaptureReady(imageCapture)
    }, ContextCompat.getMainExecutor(context))
}

private fun bindCamera(
    context: Context,
    lifecycleOwner: androidx.lifecycle.LifecycleOwner,
    imageCapture: ImageCapture?,
    onCaptureReady: (ImageCapture) -> Unit
) {
    if (imageCapture != null) {
        onCaptureReady(imageCapture)
    }
}

private fun capturePhoto(
    context: Context,
    projectPath: String,
    imageCapture: ImageCapture?,
    executor: ExecutorService,
    onSaved: (File) -> Unit
) {
    val capture = imageCapture ?: return
    val folder = File(projectPath, "raw").also { it.mkdirs() }
    val timeStamp = SimpleDateFormat("yyyyMMdd_HHmmss", Locale.US).format(Date())
    val file = File(folder, "IMG_$timeStamp.jpg")
    val output = ImageCapture.OutputFileOptions.Builder(file).build()
    capture.takePicture(
        output,
        executor,
        object : ImageCapture.OnImageSavedCallback {
            override fun onImageSaved(outputFileResults: ImageCapture.OutputFileResults) {
                onSaved(file)
            }

            override fun onError(exception: ImageCaptureException) {
                exception.printStackTrace()
            }
        }
    )
}
