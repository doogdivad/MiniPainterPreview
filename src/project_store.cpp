#include "project_store.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <random>
#include <sstream>

namespace fs = std::filesystem;

namespace mini {

namespace {

MiniResultCode to_code(bool ok) {
    return ok ? MINI_OK : MINI_ERROR_IO;
}

bool write_project_json(const fs::path& path, const ProjectMetadata& m, std::string* out_error) {
    std::ofstream out(path);
    if (!out) {
        *out_error = "failed to open project.json for writing";
        return false;
    }

    out << "{\n";
    out << "  \"project_id\": \"" << m.project_id << "\",\n";
    out << "  \"display_name\": \"" << m.display_name << "\",\n";
    out << "  \"created_at\": \"" << m.created_at << "\",\n";
    out << "  \"updated_at\": \"" << m.updated_at << "\",\n";
    out << "  \"capture_set\": [],\n";
    out << "  \"paint_schemes\": []\n";
    out << "}\n";
    return true;
}

std::string read_text(const fs::path& path) {
    std::ifstream in(path);
    if (!in) {
        return {};
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

std::string extract_json_string(const std::string& text, const std::string& key) {
    const std::string quoted_key = "\"" + key + "\"";
    auto key_pos = text.find(quoted_key);
    if (key_pos == std::string::npos) {
        return {};
    }
    auto colon = text.find(':', key_pos);
    if (colon == std::string::npos) {
        return {};
    }
    auto first_quote = text.find('"', colon);
    if (first_quote == std::string::npos) {
        return {};
    }
    auto second_quote = text.find('"', first_quote + 1);
    if (second_quote == std::string::npos) {
        return {};
    }
    return text.substr(first_quote + 1, second_quote - first_quote - 1);
}

}  // namespace

ProjectStore::ProjectStore(std::string root_dir, std::string project_dir, ProjectMetadata metadata)
    : root_dir_(std::move(root_dir)), project_dir_(std::move(project_dir)), metadata_(std::move(metadata)) {}

MiniResult ProjectStore::create(const std::string& root_dir, const std::string& display_name, ProjectStore** out_store, std::string* out_error) {
    if (root_dir.empty() || display_name.empty() || out_store == nullptr || out_error == nullptr) {
        return {MINI_ERROR_INVALID_ARGUMENT};
    }

    const std::string id = generate_id();
    const fs::path project_dir = fs::path(root_dir) / "minis" / id;

    std::error_code ec;
    fs::create_directories(project_dir / "raw", ec);
    fs::create_directories(project_dir / "masks", ec);
    fs::create_directories(project_dir / "processed", ec);
    fs::create_directories(project_dir / "preview", ec);
    fs::create_directories(project_dir / "schemes" / "layer_masks", ec);
    if (ec) {
        *out_error = "failed to create project directories";
        return {MINI_ERROR_IO};
    }

    ProjectMetadata metadata;
    metadata.project_id = id;
    metadata.display_name = display_name;
    metadata.created_at = utc_now_iso8601();
    metadata.updated_at = metadata.created_at;

    if (!write_project_json(project_dir / "project.json", metadata, out_error)) {
        return {MINI_ERROR_IO};
    }

    *out_store = new ProjectStore(root_dir, project_dir.string(), metadata);
    return {MINI_OK};
}

MiniResult ProjectStore::open(const std::string& project_dir, ProjectStore** out_store, std::string* out_error) {
    if (project_dir.empty() || out_store == nullptr || out_error == nullptr) {
        return {MINI_ERROR_INVALID_ARGUMENT};
    }

    const fs::path project_json = fs::path(project_dir) / "project.json";
    if (!fs::exists(project_json)) {
        *out_error = "project.json not found";
        return {MINI_ERROR_FILE_NOT_FOUND};
    }

    std::string text = read_text(project_json);
    if (text.empty()) {
        *out_error = "unable to read project.json";
        return {MINI_ERROR_PROJECT_CORRUPT};
    }

    ProjectMetadata metadata;
    metadata.project_id = extract_json_string(text, "project_id");
    metadata.display_name = extract_json_string(text, "display_name");
    metadata.created_at = extract_json_string(text, "created_at");
    metadata.updated_at = extract_json_string(text, "updated_at");

    if (metadata.project_id.empty()) {
        *out_error = "invalid or corrupt project.json";
        return {MINI_ERROR_PROJECT_CORRUPT};
    }

    *out_store = new ProjectStore(fs::path(project_dir).parent_path().parent_path().string(), project_dir, metadata);
    return {MINI_OK};
}

MiniResult ProjectStore::save(std::string* out_error) const {
    if (out_error == nullptr) {
        return {MINI_ERROR_INVALID_ARGUMENT};
    }
    const bool ok = write_project_json(fs::path(project_dir_) / "project.json", metadata_, out_error);
    return {to_code(ok)};
}

std::string utc_now_iso8601() {
    auto now = std::chrono::system_clock::now();
    std::time_t tt = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32)
    gmtime_s(&tm, &tt);
#else
    gmtime_r(&tt, &tm);
#endif
    std::ostringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

std::string generate_id() {
    static thread_local std::mt19937_64 rng{std::random_device{}()};
    std::uniform_int_distribution<uint64_t> dist;
    uint64_t v = dist(rng);
    std::ostringstream ss;
    ss << std::hex << v;
    return ss.str();
}

}  // namespace mini
