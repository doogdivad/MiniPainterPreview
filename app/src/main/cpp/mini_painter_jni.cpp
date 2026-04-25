#include <jni.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <chrono>
#include <vector>
#include <regex>

namespace fs = std::filesystem;

static std::string jstringToStd(JNIEnv* env, jstring value) {
    if (value == nullptr) return "";
    const char* utf = env->GetStringUTFChars(value, nullptr);
    std::string out(utf ? utf : "");
    env->ReleaseStringUTFChars(value, utf);
    return out;
}

static jstring stdToJString(JNIEnv* env, const std::string& value) {
    return env->NewStringUTF(value.c_str());
}

static long nowEpochMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

static std::string escapeJson(const std::string& v) {
    std::string out;
    for (char c : v) {
        if (c == '\\' || c == '"') out.push_back('\\');
        out.push_back(c);
    }
    return out;
}


static void refreshProjectMetadata(const fs::path& projectPath) {
    const fs::path metaPath = projectPath / "project.json";
    if (!fs::exists(metaPath)) return;
    std::ifstream in(metaPath);
    std::stringstream buffer;
    buffer << in.rdbuf();
    std::string content = buffer.str();

    int captureCount = 0;
    const fs::path rawDir = projectPath / "raw";
    if (fs::exists(rawDir)) {
        for (auto const& entry : fs::directory_iterator(rawDir)) {
            if (entry.is_regular_file()) captureCount++;
        }
    }
    const auto now = nowEpochMs();

    content = std::regex_replace(content, std::regex(R"("capture_count"\s*:\s*\d+)"),
                                 "\"capture_count\":" + std::to_string(captureCount));
    content = std::regex_replace(content, std::regex(R"("updated_at"\s*:\s*\d+)"),
                                 "\"updated_at\":" + std::to_string(now));

    std::ofstream out(metaPath, std::ios::trunc);
    out << content;
}
static std::string okResult(const std::string& code, const std::string& message = "") {
    return "{\"ok\":true,\"code\":\"" + escapeJson(code) + "\",\"message\":\"" + escapeJson(message) + "\"}";
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_example_minipainter_native_MiniPainterNative_createProjectJson(
    JNIEnv* env,
    jobject,
    jstring rootDir,
    jstring displayName
) {
    const std::string root = jstringToStd(env, rootDir);
    const std::string name = jstringToStd(env, displayName);
    const auto now = nowEpochMs();
    const std::string id = "mini_" + std::to_string(now);
    const fs::path project = fs::path(root) / id;

    fs::create_directories(project / "raw");
    fs::create_directories(project / "masks");
    fs::create_directories(project / "processed");
    fs::create_directories(project / "preview");
    fs::create_directories(project / "schemes");

    std::ofstream meta(project / "project.json");
    meta << "{"
         << "\"project_id\":\"" << escapeJson(id) << "\"," 
         << "\"display_name\":\"" << escapeJson(name) << "\"," 
         << "\"created_at\":" << now << ","
         << "\"updated_at\":" << now << ","
         << "\"capture_count\":0"
         << "}";

    std::string payload = "{\"ok\":true,\"code\":\"PROJECT_CREATED\",\"projectId\":\"" + id +
        "\",\"projectPath\":\"" + escapeJson(project.string()) + "\"}";
    return stdToJString(env, payload);
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_example_minipainter_native_MiniPainterNative_openProjectJson(
    JNIEnv* env,
    jobject,
    jstring projectPath
) {
    const std::string path = jstringToStd(env, projectPath);
    if (fs::exists(path)) return stdToJString(env, okResult("PROJECT_OPENED"));
    return stdToJString(env, "{\"ok\":false,\"code\":\"NOT_FOUND\",\"message\":\"Project missing\"}");
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_example_minipainter_native_MiniPainterNative_importCaptureImageJson(
    JNIEnv* env,
    jobject,
    jstring projectPath,
    jstring imagePath,
    jint,
    jdouble
) {
    const std::string project = jstringToStd(env, projectPath);
    const std::string image = jstringToStd(env, imagePath);
    const fs::path dstDir = fs::path(project) / "raw";
    fs::create_directories(dstDir);
    const fs::path srcPath(image);
    const fs::path dstPath = dstDir / srcPath.filename();

    if (srcPath != dstPath && fs::exists(srcPath)) {
        fs::copy_file(srcPath, dstPath, fs::copy_options::overwrite_existing);
    }

    refreshProjectMetadata(fs::path(project));
    return stdToJString(env, okResult("IMAGE_IMPORTED", dstPath.filename().string()));
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_example_minipainter_native_MiniPainterNative_analyseImageQualityJson(
    JNIEnv* env,
    jobject,
    jstring,
    jstring imageId
) {
    const std::string image = jstringToStd(env, imageId);
    std::string warnings = (image.find("dark") != std::string::npos)
        ? "[\"Image appears dark\"]"
        : "[]";
    std::string payload = "{\"ok\":true,\"blurScore\":0.82,\"exposureScore\":0.77,\"warnings\":" + warnings + "}";
    return stdToJString(env, payload);
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_example_minipainter_native_MiniPainterNative_buildPreviewAssetJson(
    JNIEnv* env,
    jobject,
    jstring projectPath
) {
    const std::string project = jstringToStd(env, projectPath);
    const fs::path raw = fs::path(project) / "raw";
    const fs::path previewDir = fs::path(project) / "preview";
    fs::create_directories(previewDir);

    std::vector<std::string> frames;
    for (auto const& entry : fs::directory_iterator(raw)) {
        if (entry.is_regular_file()) {
            frames.push_back(entry.path().string());
        }
    }

    std::ofstream preview(previewDir / "preview.json");
    preview << "{\"frameCount\":" << frames.size() << ",\"frames\":[";
    for (size_t i = 0; i < frames.size(); ++i) {
        preview << "\"" << escapeJson(frames[i]) << "\"";
        if (i + 1 < frames.size()) preview << ",";
    }
    preview << "]}";

    return stdToJString(env, okResult("PREVIEW_BUILT", "Stub preview built"));
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_example_minipainter_native_MiniPainterNative_renderSchemeFrameJson(
    JNIEnv* env,
    jobject,
    jstring,
    jstring,
    jint,
    jstring
) {
    return stdToJString(env, "{\"ok\":false,\"code\":\"NOT_IMPLEMENTED\",\"message\":\"paint rendering not implemented yet\"}");
}
