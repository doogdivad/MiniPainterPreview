#include "project_store.hpp"

#include <chrono>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <optional>
#include <random>
#include <regex>
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
    out << "  \"capture_set\": [\n";
    for (size_t i = 0; i < m.capture_set.size(); ++i) {
        const auto& image = m.capture_set[i];
        out << "    {\n";
        out << "      \"image_id\": " << image.image_id << ",\n";
        out << "      \"file_path\": \"" << image.file_path << "\",\n";
        out << "      \"angle_index\": " << image.angle_index << ",\n";
        out << "      \"estimated_angle_degrees\": " << image.estimated_angle_degrees << ",\n";
        out << "      \"quality_score\": " << image.quality_score << ",\n";
        out << "      \"mask_path\": \"" << image.mask_path << "\",\n";
        out << "      \"processed_image_path\": \"" << image.processed_image_path << "\"\n";
        out << "    }";
        if (i + 1 != m.capture_set.size()) {
            out << ",";
        }
        out << "\n";
    }
    out << "  ],\n";
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

std::string extract_json_string_from_block(const std::string& text, const std::string& key) {
    std::regex pattern("\"" + key + "\"\\s*:\\s*\"([^\"]*)\"");
    std::smatch match;
    if (!std::regex_search(text, match, pattern) || match.size() < 2) {
        return {};
    }
    return match[1].str();
}

std::optional<uint64_t> extract_json_uint64_from_block(const std::string& text, const std::string& key) {
    std::regex pattern("\"" + key + "\"\\s*:\\s*([0-9]+)");
    std::smatch match;
    if (!std::regex_search(text, match, pattern) || match.size() < 2) {
        return std::nullopt;
    }
    std::stringstream ss(match[1].str());
    uint64_t value = 0;
    ss >> value;
    if (!ss) {
        return std::nullopt;
    }
    return value;
}

std::optional<int> extract_json_int_from_block(const std::string& text, const std::string& key) {
    std::regex pattern("\"" + key + "\"\\s*:\\s*(-?[0-9]+)");
    std::smatch match;
    if (!std::regex_search(text, match, pattern) || match.size() < 2) {
        return std::nullopt;
    }
    std::stringstream ss(match[1].str());
    int value = 0;
    ss >> value;
    if (!ss) {
        return std::nullopt;
    }
    return value;
}

std::optional<double> extract_json_double_from_block(const std::string& text, const std::string& key) {
    std::regex pattern("\"" + key + "\"\\s*:\\s*(-?[0-9]+(?:\\.[0-9]+)?)");
    std::smatch match;
    if (!std::regex_search(text, match, pattern) || match.size() < 2) {
        return std::nullopt;
    }
    std::stringstream ss(match[1].str());
    double value = 0;
    ss >> value;
    if (!ss) {
        return std::nullopt;
    }
    return value;
}

std::string to_lower(std::string value) {
    for (char& c : value) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return value;
}

bool is_supported_image_extension(const std::string& ext) {
    const std::string normalized = to_lower(ext);
    return normalized == ".jpg" || normalized == ".jpeg" || normalized == ".png";
}

std::vector<ProjectMetadata::CaptureImage> parse_capture_set(const std::string& text) {
    std::vector<ProjectMetadata::CaptureImage> capture_set;

    const std::string key = "\"capture_set\"";
    auto capture_pos = text.find(key);
    if (capture_pos == std::string::npos) {
        return capture_set;
    }
    auto array_start = text.find('[', capture_pos);
    if (array_start == std::string::npos) {
        return capture_set;
    }
    auto array_end = text.find(']', array_start);
    if (array_end == std::string::npos) {
        return capture_set;
    }

    const std::string array_text = text.substr(array_start + 1, array_end - array_start - 1);
    std::regex object_pattern("\\{[^\\{\\}]*\\}");
    auto begin = std::sregex_iterator(array_text.begin(), array_text.end(), object_pattern);
    auto end = std::sregex_iterator();
    for (auto it = begin; it != end; ++it) {
        const std::string object_text = it->str();
        auto image_id = extract_json_uint64_from_block(object_text, "image_id");
        auto angle_index = extract_json_int_from_block(object_text, "angle_index");
        auto angle_degrees = extract_json_double_from_block(object_text, "estimated_angle_degrees");
        auto quality_score = extract_json_double_from_block(object_text, "quality_score");
        std::string file_path = extract_json_string_from_block(object_text, "file_path");
        if (!image_id.has_value() || !angle_index.has_value() || !angle_degrees.has_value() || !quality_score.has_value() || file_path.empty()) {
            continue;
        }

        ProjectMetadata::CaptureImage image{};
        image.image_id = *image_id;
        image.file_path = file_path;
        image.angle_index = *angle_index;
        image.estimated_angle_degrees = *angle_degrees;
        image.quality_score = *quality_score;
        image.mask_path = extract_json_string_from_block(object_text, "mask_path");
        image.processed_image_path = extract_json_string_from_block(object_text, "processed_image_path");
        capture_set.push_back(std::move(image));
    }
    return capture_set;
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
    metadata.capture_set = {};

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
    metadata.capture_set = parse_capture_set(text);

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

MiniResult ProjectStore::import_capture_image(
    const std::string& source_image_path,
    int angle_index,
    double estimated_angle_degrees,
    uint64_t* out_image_id,
    std::string* out_error) {
    if (source_image_path.empty() || out_image_id == nullptr || out_error == nullptr) {
        return {MINI_ERROR_INVALID_ARGUMENT};
    }

    const fs::path source_path(source_image_path);
    if (!fs::exists(source_path)) {
        *out_error = "source image path does not exist";
        return {MINI_ERROR_FILE_NOT_FOUND};
    }
    if (!fs::is_regular_file(source_path)) {
        *out_error = "source image path is not a file";
        return {MINI_ERROR_INVALID_ARGUMENT};
    }

    const std::string ext = to_lower(source_path.extension().string());
    if (!is_supported_image_extension(ext)) {
        *out_error = "unsupported image extension; only JPEG and PNG are supported";
        return {MINI_ERROR_INVALID_ARGUMENT};
    }

    uint64_t max_image_id = 0;
    for (const auto& existing : metadata_.capture_set) {
        if (existing.image_id > max_image_id) {
            max_image_id = existing.image_id;
        }
    }
    const uint64_t next_image_id = max_image_id + 1;
    if (next_image_id == 0 || next_image_id == std::numeric_limits<uint64_t>::max()) {
        *out_error = "unable to generate image id";
        return {MINI_ERROR_PROCESSING};
    }

    const fs::path raw_dir = fs::path(project_dir_) / "raw";
    std::ostringstream name_builder;
    name_builder << "image_" << std::setfill('0') << std::setw(3) << next_image_id << ext;
    const std::string filename = name_builder.str();
    const fs::path destination = raw_dir / filename;
    if (fs::exists(destination)) {
        *out_error = "destination image path already exists";
        return {MINI_ERROR_IO};
    }

    std::error_code ec;
    fs::copy_file(source_path, destination, fs::copy_options::none, ec);
    if (ec) {
        *out_error = "failed to copy image into project raw directory";
        return {MINI_ERROR_IO};
    }

    ProjectMetadata::CaptureImage image{};
    image.image_id = next_image_id;
    image.file_path = (fs::path("raw") / filename).generic_string();
    image.angle_index = angle_index;
    image.estimated_angle_degrees = estimated_angle_degrees;
    image.quality_score = 0.0;
    metadata_.capture_set.push_back(std::move(image));
    metadata_.updated_at = utc_now_iso8601();

    MiniResult save_result = save(out_error);
    if (save_result.code != MINI_OK) {
        return save_result;
    }

    *out_image_id = next_image_id;
    return {MINI_OK};
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
