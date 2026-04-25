#include <iostream>
#include <string>

#include "mini_painter/mini_painter.h"

namespace {

int usage() {
    std::cerr << "Usage:\n";
    std::cerr << "  mini_painter_cli create --root <dir> --name <display_name>\n";
    std::cerr << "  mini_painter_cli import --project <project_dir> --image <path> --angle <index> [--degrees <value>]\n";
    std::cerr << "  mini_painter_cli analyse --project <project_dir> --image-id <id>\n";
    std::cerr << "  mini_painter_cli set-mask --project <project_dir> --image-id <id> --mask <path>\n";
    std::cerr << "  mini_painter_cli cleanup-mask --project <project_dir> --image-id <id> [--remove-islands <px>] [--fill-holes <0|1>] [--feather <px>] [--dilate-erode <-n..n>]\n";
    std::cerr << "  mini_painter_cli generate-frame --project <project_dir> --image-id <id> [--normalize-canvas <0|1>]\n";
    std::cerr << "  mini_painter_cli build-preview --project <project_dir> [--minimum-frames <count>]\n";
    return 1;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        return usage();
    }

    std::string command = argv[1];
    if (command == "create") {
        std::string root;
        std::string name;
        for (int i = 2; i + 1 < argc; i += 2) {
            std::string key = argv[i];
            std::string value = argv[i + 1];
            if (key == "--root") {
                root = value;
            } else if (key == "--name") {
                name = value;
            } else {
                std::cerr << "Unknown option: " << key << "\n";
                return usage();
            }
        }

        if (root.empty() || name.empty()) {
            return usage();
        }

        MiniProjectHandle project = nullptr;
        MiniResult result = mini_create_project(root.c_str(), name.c_str(), &project);
        if (result.code != MINI_OK) {
            std::cerr << "create failed: " << mini_get_last_error() << "\n";
            return 2;
        }

        result = mini_save_project(project);
        mini_close_project(project);

        if (result.code != MINI_OK) {
            std::cerr << "save failed: " << mini_get_last_error() << "\n";
            return 3;
        }

        std::cout << "Project created successfully.\n";
        return 0;
    }

    if (command == "import") {
        std::string project_path;
        std::string image_path;
        std::string angle_index;
        std::string degrees = "0";
        for (int i = 2; i + 1 < argc; i += 2) {
            std::string key = argv[i];
            std::string value = argv[i + 1];
            if (key == "--project") {
                project_path = value;
            } else if (key == "--image") {
                image_path = value;
            } else if (key == "--angle") {
                angle_index = value;
            } else if (key == "--degrees") {
                degrees = value;
            } else {
                std::cerr << "Unknown option: " << key << "\n";
                return usage();
            }
        }

        if (project_path.empty() || image_path.empty() || angle_index.empty()) {
            return usage();
        }

        MiniProjectHandle project = nullptr;
        MiniResult result = mini_open_project(project_path.c_str(), &project);
        if (result.code != MINI_OK) {
            std::cerr << "open failed: " << mini_get_last_error() << "\n";
            return 2;
        }

        MiniImageId image_id = 0;
        result = mini_import_capture_image(project, image_path.c_str(), std::stoi(angle_index), std::stod(degrees), &image_id);
        mini_close_project(project);

        if (result.code != MINI_OK) {
            std::cerr << "import failed: " << mini_get_last_error() << "\n";
            return 3;
        }

        std::cout << "Image imported with id " << image_id << ".\n";
        return 0;
    }

    if (command == "analyse") {
        std::string project_path;
        std::string image_id_text;
        for (int i = 2; i + 1 < argc; i += 2) {
            std::string key = argv[i];
            std::string value = argv[i + 1];
            if (key == "--project") {
                project_path = value;
            } else if (key == "--image-id") {
                image_id_text = value;
            } else {
                std::cerr << "Unknown option: " << key << "\n";
                return usage();
            }
        }

        if (project_path.empty() || image_id_text.empty()) {
            return usage();
        }

        MiniProjectHandle project = nullptr;
        MiniResult result = mini_open_project(project_path.c_str(), &project);
        if (result.code != MINI_OK) {
            std::cerr << "open failed: " << mini_get_last_error() << "\n";
            return 2;
        }

        MiniImageQualityReport report{};
        result = mini_analyse_image_quality(project, static_cast<MiniImageId>(std::stoull(image_id_text)), &report);
        mini_close_project(project);
        if (result.code != MINI_OK) {
            std::cerr << "analyse failed: " << mini_get_last_error() << "\n";
            return 3;
        }

        std::cout << "Quality report:\n";
        std::cout << "  blur_score: " << report.blur_score << "\n";
        std::cout << "  exposure_score: " << report.exposure_score << "\n";
        std::cout << "  width: " << report.width << "\n";
        std::cout << "  height: " << report.height << "\n";
        std::cout << "  subject_coverage_estimate: " << report.subject_coverage_estimate << "\n";
        std::cout << "  warning_flags: " << report.warning_flags << "\n";
        return 0;
    }

    if (command == "set-mask") {
        std::string project_path;
        std::string image_id_text;
        std::string mask_path;
        for (int i = 2; i + 1 < argc; i += 2) {
            std::string key = argv[i];
            std::string value = argv[i + 1];
            if (key == "--project") {
                project_path = value;
            } else if (key == "--image-id") {
                image_id_text = value;
            } else if (key == "--mask") {
                mask_path = value;
            } else {
                std::cerr << "Unknown option: " << key << "\n";
                return usage();
            }
        }

        if (project_path.empty() || image_id_text.empty() || mask_path.empty()) {
            return usage();
        }

        MiniProjectHandle project = nullptr;
        MiniResult result = mini_open_project(project_path.c_str(), &project);
        if (result.code != MINI_OK) {
            std::cerr << "open failed: " << mini_get_last_error() << "\n";
            return 2;
        }

        result = mini_set_external_mask(project, static_cast<MiniImageId>(std::stoull(image_id_text)), mask_path.c_str());
        mini_close_project(project);
        if (result.code != MINI_OK) {
            std::cerr << "set-mask failed: " << mini_get_last_error() << "\n";
            return 3;
        }

        std::cout << "Mask imported and normalized.\n";
        return 0;
    }

    if (command == "cleanup-mask") {
        std::string project_path;
        std::string image_id_text;
        MiniMaskCleanupOptions options{};
        for (int i = 2; i + 1 < argc; i += 2) {
            std::string key = argv[i];
            std::string value = argv[i + 1];
            if (key == "--project") {
                project_path = value;
            } else if (key == "--image-id") {
                image_id_text = value;
            } else if (key == "--remove-islands") {
                options.remove_small_islands = std::stoi(value);
            } else if (key == "--fill-holes") {
                options.fill_holes = std::stoi(value);
            } else if (key == "--feather") {
                options.feather_edge = std::stoi(value);
            } else if (key == "--dilate-erode") {
                options.dilate_erode_amount = std::stoi(value);
            } else {
                std::cerr << "Unknown option: " << key << "\n";
                return usage();
            }
        }

        if (project_path.empty() || image_id_text.empty()) {
            return usage();
        }

        MiniProjectHandle project = nullptr;
        MiniResult result = mini_open_project(project_path.c_str(), &project);
        if (result.code != MINI_OK) {
            std::cerr << "open failed: " << mini_get_last_error() << "\n";
            return 2;
        }

        result = mini_cleanup_mask(project, static_cast<MiniImageId>(std::stoull(image_id_text)), options);
        mini_close_project(project);
        if (result.code != MINI_OK) {
            std::cerr << "cleanup-mask failed: " << mini_get_last_error() << "\n";
            return 3;
        }

        std::cout << "Mask cleanup finished.\n";
        return 0;
    }

    if (command == "generate-frame") {
        std::string project_path;
        std::string image_id_text;
        MiniFrameOptions options{};
        options.normalize_canvas = 1;
        for (int i = 2; i + 1 < argc; i += 2) {
            std::string key = argv[i];
            std::string value = argv[i + 1];
            if (key == "--project") {
                project_path = value;
            } else if (key == "--image-id") {
                image_id_text = value;
            } else if (key == "--normalize-canvas") {
                options.normalize_canvas = std::stoi(value);
            } else {
                std::cerr << "Unknown option: " << key << "\n";
                return usage();
            }
        }

        if (project_path.empty() || image_id_text.empty()) {
            return usage();
        }

        MiniProjectHandle project = nullptr;
        MiniResult result = mini_open_project(project_path.c_str(), &project);
        if (result.code != MINI_OK) {
            std::cerr << "open failed: " << mini_get_last_error() << "\n";
            return 2;
        }

        result = mini_generate_processed_frame(project, static_cast<MiniImageId>(std::stoull(image_id_text)), options);
        mini_close_project(project);
        if (result.code != MINI_OK) {
            std::cerr << "generate-frame failed: " << mini_get_last_error() << "\n";
            return 3;
        }

        std::cout << "Processed frame generated.\n";
        return 0;
    }


    if (command == "build-preview") {
        std::string project_path;
        MiniPreviewBuildOptions options{};
        options.minimum_frames = 1;
        for (int i = 2; i + 1 < argc; i += 2) {
            std::string key = argv[i];
            std::string value = argv[i + 1];
            if (key == "--project") {
                project_path = value;
            } else if (key == "--minimum-frames") {
                options.minimum_frames = std::stoi(value);
            } else {
                std::cerr << "Unknown option: " << key << "\n";
                return usage();
            }
        }

        if (project_path.empty()) {
            return usage();
        }

        MiniProjectHandle project = nullptr;
        MiniResult result = mini_open_project(project_path.c_str(), &project);
        if (result.code != MINI_OK) {
            std::cerr << "open failed: " << mini_get_last_error() << "\n";
            return 2;
        }

        result = mini_build_preview_asset(project, options);
        mini_close_project(project);
        if (result.code != MINI_OK) {
            std::cerr << "build-preview failed: " << mini_get_last_error() << "\n";
            return 3;
        }

        std::cout << "Preview asset generated.\n";
        return 0;
    }

    std::cerr << "Unsupported command: " << command << "\n";
    return usage();
}
