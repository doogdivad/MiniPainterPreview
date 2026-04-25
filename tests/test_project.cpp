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

    const fs::path source_a = root / "input_a.jpg";
    const fs::path source_b = root / "input_b.png";
    {
        std::ofstream out(source_a);
        out << "fake-jpeg-data";
    }
    {
        std::ofstream out(source_b);
        out << "fake-png-data";
    }

    MiniImageId image_a = 0;
    result = mini_import_capture_image(reopened, source_a.string().c_str(), 0, 0.0, &image_a);
    if (result.code != MINI_OK || image_a == 0) {
        std::cerr << "mini_import_capture_image #1 failed: " << mini_get_last_error() << "\n";
        return 1;
    }
    mini_close_project(reopened);

    MiniProjectHandle reopened_again = nullptr;
    result = mini_open_project(project_dir.string().c_str(), &reopened_again);
    if (result.code != MINI_OK || reopened_again == nullptr) {
        std::cerr << "mini_open_project (reopen 2) failed: " << mini_get_last_error() << "\n";
        return 1;
    }

    MiniImageId image_b = 0;
    result = mini_import_capture_image(reopened_again, source_b.string().c_str(), 1, 15.0, &image_b);
    if (result.code != MINI_OK || image_b != image_a + 1) {
        std::cerr << "mini_import_capture_image #2 failed: " << mini_get_last_error() << "\n";
        return 1;
    }
    mini_close_project(reopened_again);

    if (!fs::exists(project_dir / "raw" / "image_001.jpg")) {
        std::cerr << "imported JPEG was not copied to raw/\n";
        return 1;
    }
    if (!fs::exists(project_dir / "raw" / "image_002.png")) {
        std::cerr << "imported PNG was not copied to raw/\n";
        return 1;
    }

    std::ifstream in(project_dir / "project.json");
    std::string json_text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (json_text.find("\"capture_set\"") == std::string::npos ||
        json_text.find("\"image_id\": 1") == std::string::npos ||
        json_text.find("\"image_id\": 2") == std::string::npos ||
        json_text.find("\"file_path\": \"raw/image_001.jpg\"") == std::string::npos ||
        json_text.find("\"file_path\": \"raw/image_002.png\"") == std::string::npos) {
        std::cerr << "project.json does not contain expected capture metadata\n";
        return 1;
    }

    fs::remove_all(root, ec);

    std::cout << "mini_painter_tests passed\n";
    return 0;
}
