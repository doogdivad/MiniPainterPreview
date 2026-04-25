package com.example.minipainter.navigation

import android.net.Uri
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import androidx.lifecycle.viewmodel.compose.viewModel
import androidx.navigation.NavType
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.compose.rememberNavController
import androidx.navigation.navArgument
import com.example.minipainter.data.MiniProjectRepository
import com.example.minipainter.ui.screens.CaptureScreen
import com.example.minipainter.ui.screens.HomeScreen
import com.example.minipainter.ui.screens.PaintScreen
import com.example.minipainter.ui.screens.PreviewScreen
import com.example.minipainter.ui.screens.ProjectDetailScreen
import com.example.minipainter.viewmodel.CaptureViewModel
import com.example.minipainter.viewmodel.HomeViewModel
import com.example.minipainter.viewmodel.HomeViewModelFactory
import com.example.minipainter.viewmodel.ProjectViewModel
import com.example.minipainter.viewmodel.ProjectViewModelFactory

@Composable
fun MiniPainterNavHost(repository: MiniProjectRepository) {
    val navController = rememberNavController()

    NavHost(navController = navController, startDestination = NavRoutes.HOME) {
        composable(NavRoutes.HOME) {
            val vm: HomeViewModel = viewModel(factory = HomeViewModelFactory(repository))
            val state by vm.uiState.collectAsStateWithLifecycle()
            HomeScreen(
                state = state,
                onNewMini = { name ->
                    vm.createProject(name) {
                        navController.navigate("${NavRoutes.CAPTURE}/${it.projectId}/${Uri.encode(it.projectPath)}")
                    }
                },
                onOpenProject = {
                    navController.navigate("${NavRoutes.PROJECT_DETAIL}/${it.projectId}/${Uri.encode(it.projectPath)}/${Uri.encode(it.name)}/${it.captureCount}")
                },
                onRefresh = vm::refresh
            )
        }

        composable(
            "${NavRoutes.PROJECT_DETAIL}/{projectId}/{projectPath}/{name}/{captureCount}",
            arguments = listOf(
                navArgument("projectId") { type = NavType.StringType },
                navArgument("projectPath") { type = NavType.StringType },
                navArgument("name") { type = NavType.StringType },
                navArgument("captureCount") { type = NavType.IntType }
            )
        ) { backStackEntry ->
            val vm: ProjectViewModel = viewModel(factory = ProjectViewModelFactory(repository))
            val state by vm.uiState.collectAsStateWithLifecycle()
            ProjectDetailScreen(
                projectId = backStackEntry.arguments?.getString("projectId").orEmpty(),
                name = Uri.decode(backStackEntry.arguments?.getString("name").orEmpty()),
                projectPath = Uri.decode(backStackEntry.arguments?.getString("projectPath").orEmpty()),
                captureCount = backStackEntry.arguments?.getInt("captureCount") ?: 0,
                state = state,
                onContinueCapture = { path, id -> navController.navigate("${NavRoutes.CAPTURE}/$id/${Uri.encode(path)}") },
                onBuildPreview = vm::buildPreview,
                onViewPreview = { path -> navController.navigate("${NavRoutes.PREVIEW}/${Uri.encode(path)}") },
                onPaint = { path -> navController.navigate("${NavRoutes.PAINT}/${Uri.encode(path)}") }
            )
        }

        composable(
            "${NavRoutes.CAPTURE}/{projectId}/{projectPath}",
            arguments = listOf(
                navArgument("projectId") { type = NavType.StringType },
                navArgument("projectPath") { type = NavType.StringType }
            )
        ) { backStackEntry ->
            val vm: CaptureViewModel = viewModel(factory = com.example.minipainter.viewmodel.CaptureViewModelFactory(repository))
            val state by vm.uiState.collectAsStateWithLifecycle()
            CaptureScreen(
                projectId = backStackEntry.arguments?.getString("projectId").orEmpty(),
                projectPath = Uri.decode(backStackEntry.arguments?.getString("projectPath").orEmpty()),
                state = state,
                onModeChanged = vm::setMode,
                onPhotoCaptured = vm::onCaptured,
                onFinished = {
                    navController.popBackStack()
                }
            )
        }

        composable(
            "${NavRoutes.PREVIEW}/{projectPath}",
            arguments = listOf(navArgument("projectPath") { type = NavType.StringType })
        ) { backStackEntry ->
            PreviewScreen(
                projectPath = Uri.decode(backStackEntry.arguments?.getString("projectPath").orEmpty()),
                repository = repository,
                onPaint = { path -> navController.navigate("${NavRoutes.PAINT}/${Uri.encode(path)}") }
            )
        }

        composable(
            "${NavRoutes.PAINT}/{projectPath}",
            arguments = listOf(navArgument("projectPath") { type = NavType.StringType })
        ) { backStackEntry ->
            PaintScreen(projectPath = Uri.decode(backStackEntry.arguments?.getString("projectPath").orEmpty()))
        }
    }
}
