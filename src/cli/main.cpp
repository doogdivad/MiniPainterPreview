#include <iostream>
#include <string>

#include "mini_painter/mini_painter.h"

namespace {

int usage() {
    std::cerr << "Usage:\n";
    std::cerr << "  mini_painter_cli create --root <dir> --name <display_name>\n";
    std::cerr << "  mini_painter_cli import --project <project_dir> --image <path> --angle <index> [--degrees <value>]\n";
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

    std::cerr << "Unsupported command: " << command << "\n";
    return usage();
}
