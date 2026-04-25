#include <filesystem>
#include <fstream>
#include <iostream>

#include "mini_painter/mini_painter.h"

namespace fs = std::filesystem;

int main() {
    const fs::path root = fs::temp_directory_path() / "mini_painter_preview_test";
    std::error_code ec;
    fs::remove_all(root, ec);

    MiniProjectHandle handle = nullptr;
    MiniResult result = mini_create_project(root.string().c_str(), "Test Mini", &handle);
    if (result.code != MINI_OK || handle == nullptr) {
        std::cerr << "mini_create_project failed: " << mini_get_last_error() << "\n";
        return 1;
    }

    result = mini_save_project(handle);
    if (result.code != MINI_OK) {
        std::cerr << "mini_save_project failed: " << mini_get_last_error() << "\n";
        mini_close_project(handle);
        return 1;
    }

    mini_close_project(handle);

    fs::path minis_dir = root / "minis";
    if (!fs::exists(minis_dir)) {
        std::cerr << "minis directory was not created\n";
        return 1;
    }

    fs::path project_dir;
    for (const auto& entry : fs::directory_iterator(minis_dir)) {
        if (entry.is_directory()) {
            project_dir = entry.path();
            break;
        }
    }

    if (project_dir.empty()) {
        std::cerr << "project folder was not created\n";
        return 1;
    }

    if (!fs::exists(project_dir / "project.json")) {
        std::cerr << "project.json was not created\n";
        return 1;
    }

    MiniProjectHandle reopened = nullptr;
    result = mini_open_project(project_dir.string().c_str(), &reopened);
    if (result.code != MINI_OK || reopened == nullptr) {
        std::cerr << "mini_open_project failed: " << mini_get_last_error() << "\n";
        return 1;
    }

    mini_close_project(reopened);
    fs::remove_all(root, ec);

    std::cout << "mini_painter_tests passed\n";
    return 0;
}
