package com.example.minipainter.navigation

import android.net.Uri
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.rememberCoroutineScope
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import androidx.lifecycle.viewmodel.compose.viewModel
import androidx.navigation.NavType
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.compose.rememberNavController
import androidx.navigation.navArgument
import com.example.minipainter.ui.screens.CaptureScreen
import com.example.minipainter.ui.screens.HomeScreen
import com.example.minipainter.ui.screens.PaintScreen
import com.example.minipainter.ui.screens.PreviewScreen
import com.example.minipainter.ui.screens.ProjectDetailScreen
import com.example.minipainter.viewmodel.CaptureMode
import com.example.minipainter.viewmodel.CaptureViewModel
import com.example.minipainter.viewmodel.HomeViewModel
import com.example.minipainter.viewmodel.MiniPainterViewModelFactory
import com.example.minipainter.viewmodel.ProjectViewModel
import kotlinx.coroutines.launch

@Composable
fun MiniPainterNavGraph(factory: MiniPainterViewModelFactory) {
    val navController = rememberNavController()
    val scope = rememberCoroutineScope()

    NavHost(navController = navController, startDestination = AppRoutes.HOME) {
        composable(AppRoutes.HOME) {
            val vm: HomeViewModel = viewModel(factory = factory)
            val state by vm.uiState.collectAsStateWithLifecycle()
            HomeScreen(
                state = state,
                onRefresh = vm::refresh,
                onCreateProject = { name ->
                    scope.launch {
                        val path = vm.createProject(name)
                        path?.let {
                            navController.navigate("${AppRoutes.CAPTURE}/${Uri.encode(it)}/${CaptureMode.BETTER.name}")
                        }
                    }
                },
                onOpenProject = { projectPath ->
                    navController.navigate("${AppRoutes.PROJECT_DETAIL}/${Uri.encode(projectPath)}")
                }
            )
        }

        composable(
            "${AppRoutes.PROJECT_DETAIL}/{projectPath}",
            arguments = listOf(navArgument("projectPath") { type = NavType.StringType })
        ) { backStackEntry ->
            val vm: ProjectViewModel = viewModel(factory = factory)
            val state by vm.uiState.collectAsStateWithLifecycle()
            val projectPath = Uri.decode(backStackEntry.arguments?.getString("projectPath").orEmpty())
            LaunchedEffect(projectPath) { vm.load(projectPath) }
            ProjectDetailScreen(
                state = state,
                onContinueCapture = {
                    navController.navigate("${AppRoutes.CAPTURE}/${Uri.encode(projectPath)}/${CaptureMode.BETTER.name}")
                },
                onBuildPreview = vm::buildPreview,
                onViewPreview = { navController.navigate("${AppRoutes.PREVIEW}/${Uri.encode(projectPath)}") },
                onPaint = { navController.navigate("${AppRoutes.PAINT}/${Uri.encode(projectPath)}") }
            )
        }

        composable(
            "${AppRoutes.CAPTURE}/{projectPath}/{mode}",
            arguments = listOf(
                navArgument("projectPath") { type = NavType.StringType },
                navArgument("mode") { type = NavType.StringType }
            )
        ) { entry ->
            val vm: CaptureViewModel = viewModel(factory = factory)
            val state by vm.uiState.collectAsStateWithLifecycle()
            val projectPath = Uri.decode(entry.arguments?.getString("projectPath").orEmpty())
            val mode = entry.arguments?.getString("mode")?.let(CaptureMode::valueOf) ?: CaptureMode.BETTER
            LaunchedEffect(projectPath, mode) { vm.loadProject(projectPath, mode) }
            CaptureScreen(
                state = state,
                onPhotoCaptured = vm::onPhotoCaptured,
                onFinish = { navController.popBackStack() }
            )
        }

        composable(
            "${AppRoutes.PREVIEW}/{projectPath}",
            arguments = listOf(navArgument("projectPath") { type = NavType.StringType })
        ) { entry ->
            val vm: ProjectViewModel = viewModel(factory = factory)
            val state by vm.uiState.collectAsStateWithLifecycle()
            val projectPath = Uri.decode(entry.arguments?.getString("projectPath").orEmpty())
            LaunchedEffect(projectPath) { vm.load(projectPath) }
            PreviewScreen(
                preview = state.preview,
                onPaint = { navController.navigate("${AppRoutes.PAINT}/${Uri.encode(projectPath)}") }
            )
        }

        composable(
            "${AppRoutes.PAINT}/{projectPath}",
            arguments = listOf(navArgument("projectPath") { type = NavType.StringType })
        ) { entry ->
            PaintScreen(projectPath = Uri.decode(entry.arguments?.getString("projectPath").orEmpty()))
        }
    }
}
