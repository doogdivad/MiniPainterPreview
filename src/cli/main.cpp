#include <iostream>
#include <string>

#include "mini_painter/mini_painter.h"

namespace {

int usage() {
    std::cerr << "Usage:\n";
    std::cerr << "  mini_painter_cli create --root <dir> --name <display_name>\n";
    return 1;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        return usage();
    }

    std::string command = argv[1];
    if (command != "create") {
        std::cerr << "Unsupported command: " << command << "\n";
        return usage();
    }

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
