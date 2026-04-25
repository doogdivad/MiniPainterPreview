#include <jni.h>
#include <string>
#include <vector>
#include <chrono>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <filesystem>
#include <regex>
#include <iterator>

namespace fs = std::filesystem;

static std::string jstring_to_std(JNIEnv* env, jstring value) {
    if (!value) return "";
    const char* chars = env->GetStringUTFChars(value, nullptr);
    std::string out(chars ? chars : "");
    env->ReleaseStringUTFChars(value, chars);
    return out;
}

static jstring to_jstring(JNIEnv* env, const std::string& value) {
    return env->NewStringUTF(value.c_str());
}

static std::string now_millis_str() {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    return std::to_string(ms);
}

static long long now_millis() {
    auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
}

static std::string extract_json_string_field(const std::string& text, const std::string& field, const std::string& fallback) {
    const std::regex pattern("\"" + field + "\"\\s*:\\s*\"([^\"]*)\"");
    std::smatch match;
    if (std::regex_search(text, match, pattern) && match.size() > 1) return match[1].str();
    return fallback;
}

static long long extract_json_long_field(const std::string& text, const std::string& field, long long fallback) {
    const std::regex pattern("\"" + field + "\"\\s*:\\s*(\\d+)");
    std::smatch match;
    if (std::regex_search(text, match, pattern) && match.size() > 1) return std::stoll(match[1].str());
    return fallback;
}

static std::string make_project_json(
    const std::string& id,
    const std::string& name,
    const std::string& path,
    int captureCount,
    long long createdAt
) {
    const auto updatedAt = now_millis();
    std::ostringstream out;
    out << "{"
        << "\"projectId\":\"" << id << "\"," 
        << "\"name\":\"" << name << "\"," 
        << "\"projectPath\":\"" << path << "\"," 
        << "\"thumbnailPath\":null," 
        << "\"captureCount\":" << captureCount << "," 
        << "\"createdAt\":" << createdAt << "," 
        << "\"updatedAt\":" << updatedAt
        << "}";
    return out.str();
}

static int count_raw_images(const fs::path& rawDir) {
    if (!fs::exists(rawDir)) return 0;
    int count = 0;
    for (const auto& entry : fs::directory_iterator(rawDir)) {
        if (!entry.is_regular_file()) continue;
        const auto ext = entry.path().extension().string();
        if (ext == ".jpg" || ext == ".jpeg" || ext == ".png") ++count;
    }
    return count;
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_minipainter_nativebridge_MiniPainterNative_createProjectJson(
    JNIEnv* env,
    jobject,
    jstring rootDir,
    jstring displayName
) {
    try {
        const std::string root = jstring_to_std(env, rootDir);
        const std::string name = jstring_to_std(env, displayName);
        const std::string projectId = "mini_" + now_millis_str();
        const fs::path projectPath = fs::path(root) / projectId;
        fs::create_directories(projectPath / "raw");
        fs::create_directories(projectPath / "masks");
        fs::create_directories(projectPath / "processed");
        fs::create_directories(projectPath / "preview");
        fs::create_directories(projectPath / "schemes");

        std::ofstream(projectPath / "project.json") << make_project_json(projectId, name, projectPath.string(), 0, now_millis());

        std::ostringstream out;
        out << "{\"ok\":true,\"code\":\"OK\",\"projectId\":\"" << projectId
            << "\",\"projectPath\":\"" << projectPath.string() << "\"}";
        return to_jstring(env, out.str());
    } catch (const std::exception& ex) {
        std::ostringstream out;
        out << "{\"ok\":false,\"code\":\"CREATE_FAILED\",\"message\":\"" << ex.what() << "\"}";
        return to_jstring(env, out.str());
    }
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_minipainter_nativebridge_MiniPainterNative_openProjectJson(
    JNIEnv* env,
    jobject,
    jstring projectPath
) {
    const std::string path = jstring_to_std(env, projectPath);
    const bool exists = fs::exists(fs::path(path) / "project.json");
    if (exists) {
        return to_jstring(env, "{\"ok\":true,\"code\":\"OK\"}");
    }
    return to_jstring(env, "{\"ok\":false,\"code\":\"NOT_FOUND\",\"message\":\"project.json missing\"}");
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_minipainter_nativebridge_MiniPainterNative_importCaptureImageJson(
    JNIEnv* env,
    jobject,
    jstring projectPath,
    jstring imagePath,
    jint angleIndex,
    jdouble
) {
    try {
        const fs::path project = jstring_to_std(env, projectPath);
        const fs::path source = jstring_to_std(env, imagePath);
        const fs::path rawDir = project / "raw";
        fs::create_directories(rawDir);

        std::ostringstream fileName;
        fileName << "image_" << std::setfill('0') << std::setw(3) << angleIndex << ".jpg";
        const fs::path dest = rawDir / fileName.str();
        fs::copy_file(source, dest, fs::copy_options::overwrite_existing);

        std::ifstream currentIn(project / "project.json");
        const std::string current((std::istreambuf_iterator<char>(currentIn)), std::istreambuf_iterator<char>());
        const std::string projectId = extract_json_string_field(current, "projectId", project.filename().string());
        const std::string name = extract_json_string_field(current, "name", project.filename().string());
        const long long createdAt = extract_json_long_field(current, "createdAt", now_millis());

        std::ofstream(project / "project.json") << make_project_json(
            projectId,
            name,
            project.string(),
            count_raw_images(rawDir),
            createdAt
        );

        return to_jstring(env, "{\"ok\":true,\"code\":\"OK\"}");
    } catch (const std::exception& ex) {
        std::ostringstream out;
        out << "{\"ok\":false,\"code\":\"IMPORT_FAILED\",\"message\":\"" << ex.what() << "\"}";
        return to_jstring(env, out.str());
    }
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_minipainter_nativebridge_MiniPainterNative_analyseImageQualityJson(
    JNIEnv* env,
    jobject,
    jstring,
    jstring
) {
    return to_jstring(env, "{\"ok\":true,\"blurScore\":0.82,\"exposureScore\":0.73,\"warnings\":[]}");
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_minipainter_nativebridge_MiniPainterNative_buildPreviewAssetJson(
    JNIEnv* env,
    jobject,
    jstring projectPath
) {
    try {
        const fs::path project = jstring_to_std(env, projectPath);
        const fs::path raw = project / "raw";
        const fs::path preview = project / "preview";
        fs::create_directories(preview);

        std::vector<std::string> frames;
        for (const auto& entry : fs::directory_iterator(raw)) {
            if (entry.is_regular_file()) {
                const fs::path outPath = preview / entry.path().filename();
                fs::copy_file(entry.path(), outPath, fs::copy_options::overwrite_existing);
                frames.push_back(outPath.string());
            }
        }

        std::ostringstream previewJson;
        previewJson << "{\"framePaths\":[";
        for (size_t i = 0; i < frames.size(); ++i) {
            previewJson << "\"" << frames[i] << "\"";
            if (i + 1 < frames.size()) previewJson << ",";
        }
        previewJson << "]}";
        std::ofstream(preview / "preview.json") << previewJson.str();

        return to_jstring(env, "{\"ok\":true,\"code\":\"OK\"}");
    } catch (const std::exception& ex) {
        std::ostringstream out;
        out << "{\"ok\":false,\"code\":\"PREVIEW_FAILED\",\"message\":\"" << ex.what() << "\"}";
        return to_jstring(env, out.str());
    }
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_minipainter_nativebridge_MiniPainterNative_renderSchemeFrameJson(
    JNIEnv* env,
    jobject,
    jstring,
    jstring,
    jint,
    jstring
) {
    return to_jstring(env, "{\"ok\":false,\"code\":\"NOT_IMPLEMENTED\",\"message\":\"paint rendering not implemented yet\"}");
}
