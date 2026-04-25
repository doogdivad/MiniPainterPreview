#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#if defined(MINI_PAINTER_HAS_OPENCV)
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#endif

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
    const fs::path source_corrupt = root / "input_corrupt.jpg";

#if defined(MINI_PAINTER_HAS_OPENCV)
    cv::Mat clear_image(640, 640, CV_8UC3, cv::Scalar(10, 10, 10));
    cv::rectangle(clear_image, cv::Point(180, 180), cv::Point(460, 460), cv::Scalar(230, 230, 230), cv::FILLED);
    if (!cv::imwrite(source_a.string(), clear_image)) {
        std::cerr << "failed to write source_a image fixture\n";
        return 1;
    }

    cv::Mat dark_image(400, 400, CV_8UC3, cv::Scalar(8, 8, 8));
    if (!cv::imwrite(source_b.string(), dark_image)) {
        std::cerr << "failed to write source_b image fixture\n";
        return 1;
    }
#else
    {
        std::ofstream out(source_a);
        out << "fake-jpeg-data";
    }
    {
        std::ofstream out(source_b);
        out << "fake-png-data";
    }
#endif

    {
        std::ofstream out(source_corrupt);
        out << "not a real image";
    }

    MiniImageId image_a = 0;
    result = mini_import_capture_image(reopened, source_a.string().c_str(), 0, 0.0, &image_a);
    if (result.code != MINI_OK || image_a == 0) {
        std::cerr << "mini_import_capture_image #1 failed: " << mini_get_last_error() << "\n";
        return 1;
    }

    MiniImageQualityReport report_a{};
    result = mini_analyse_image_quality(reopened, image_a, &report_a);
#if defined(MINI_PAINTER_HAS_OPENCV)
    if (result.code != MINI_OK) {
        std::cerr << "mini_analyse_image_quality #1 failed: " << mini_get_last_error() << "\n";
        return 1;
    }
    if (report_a.width != 640 || report_a.height != 640) {
        std::cerr << "quality report size mismatch for image_a\n";
        return 1;
    }
#else
    if (result.code != MINI_ERROR_NOT_IMPLEMENTED) {
        std::cerr << "expected MINI_ERROR_NOT_IMPLEMENTED when OpenCV is unavailable\n";
        return 1;
    }
#endif

#if defined(MINI_PAINTER_HAS_OPENCV)
    const fs::path external_mask_path = root / "external_mask.png";
    cv::Mat external_mask(200, 200, CV_8UC1, cv::Scalar(0));
    cv::rectangle(external_mask, cv::Point(20, 20), cv::Point(180, 180), cv::Scalar(255), cv::FILLED);
    cv::circle(external_mask, cv::Point(100, 100), 30, cv::Scalar(0), cv::FILLED);      // large hole
    cv::rectangle(external_mask, cv::Point(2, 2), cv::Point(6, 6), cv::Scalar(255), cv::FILLED);  // tiny island
    if (!cv::imwrite(external_mask_path.string(), external_mask)) {
        std::cerr << "failed to write external mask fixture\n";
        return 1;
    }

    result = mini_set_external_mask(reopened, image_a, external_mask_path.string().c_str());
    if (result.code != MINI_OK) {
        std::cerr << "mini_set_external_mask failed: " << mini_get_last_error() << "\n";
        return 1;
    }

    MiniMaskCleanupOptions cleanup_options{};
    cleanup_options.remove_small_islands = 128;
    cleanup_options.fill_holes = 1;
    cleanup_options.feather_edge = 0;
    cleanup_options.dilate_erode_amount = 0;

    result = mini_cleanup_mask(reopened, image_a, cleanup_options);
    if (result.code != MINI_OK) {
        std::cerr << "mini_cleanup_mask failed: " << mini_get_last_error() << "\n";
        return 1;
    }

    MiniFrameOptions frame_options{};
    frame_options.normalize_canvas = 1;
    result = mini_generate_processed_frame(reopened, image_a, frame_options);
    if (result.code != MINI_OK) {
        std::cerr << "mini_generate_processed_frame failed: " << mini_get_last_error() << "\n";
        return 1;
    }

    const fs::path cleaned_mask_path = project_dir / "masks" / "mask_1.png";
    if (!fs::exists(cleaned_mask_path)) {
        std::cerr << "cleaned mask file was not written\n";
        return 1;
    }

    cv::Mat cleaned_mask = cv::imread(cleaned_mask_path.string(), cv::IMREAD_GRAYSCALE);
    if (cleaned_mask.empty()) {
        std::cerr << "failed to load cleaned mask\n";
        return 1;
    }
    if (cleaned_mask.rows != 640 || cleaned_mask.cols != 640) {
        std::cerr << "cleaned mask dimensions do not match source image\n";
        return 1;
    }

    if (cleaned_mask.at<uint8_t>(0, 0) != 0) {
        std::cerr << "small island should have been removed after cleanup\n";
        return 1;
    }
    if (cleaned_mask.at<uint8_t>(320, 320) != 255) {
        std::cerr << "hole should have been filled after cleanup\n";
        return 1;
    }

    const fs::path processed_frame_path = project_dir / "processed" / "frame_001.png";
    if (!fs::exists(processed_frame_path)) {
        std::cerr << "processed frame file was not written\n";
        return 1;
    }
    cv::Mat processed_frame = cv::imread(processed_frame_path.string(), cv::IMREAD_UNCHANGED);
    if (processed_frame.empty() || processed_frame.type() != CV_8UC4) {
        std::cerr << "processed frame should be a non-empty RGBA PNG\n";
        return 1;
    }
    if (processed_frame.rows != processed_frame.cols) {
        std::cerr << "processed frame should be normalized to a square canvas\n";
        return 1;
    }
    const cv::Vec4b top_left = processed_frame.at<cv::Vec4b>(0, 0);
    if (top_left[3] != 0) {
        std::cerr << "processed frame background should preserve transparency\n";
        return 1;
    }
#else
    result = mini_set_external_mask(reopened, image_a, source_a.string().c_str());
    if (result.code != MINI_ERROR_NOT_IMPLEMENTED) {
        std::cerr << "expected MINI_ERROR_NOT_IMPLEMENTED for mini_set_external_mask without OpenCV\n";
        return 1;
    }

    MiniMaskCleanupOptions cleanup_options{};
    result = mini_cleanup_mask(reopened, image_a, cleanup_options);
    if (result.code != MINI_ERROR_NOT_IMPLEMENTED) {
        std::cerr << "expected MINI_ERROR_NOT_IMPLEMENTED for mini_cleanup_mask without OpenCV\n";
        return 1;
    }

    MiniFrameOptions frame_options{};
    frame_options.normalize_canvas = 1;
    result = mini_generate_processed_frame(reopened, image_a, frame_options);
    if (result.code != MINI_ERROR_NOT_IMPLEMENTED) {
        std::cerr << "expected MINI_ERROR_NOT_IMPLEMENTED for mini_generate_processed_frame without OpenCV\n";
        return 1;
    }
#endif

    MiniImageId image_b = 0;
    result = mini_import_capture_image(reopened, source_b.string().c_str(), 1, 15.0, &image_b);
    if (result.code != MINI_OK || image_b != image_a + 1) {
        std::cerr << "mini_import_capture_image #2 failed: " << mini_get_last_error() << "\n";
        return 1;
    }

    MiniImageQualityReport report_b{};
    result = mini_analyse_image_quality(reopened, image_b, &report_b);
#if defined(MINI_PAINTER_HAS_OPENCV)
    if (result.code != MINI_OK) {
        std::cerr << "mini_analyse_image_quality #2 failed: " << mini_get_last_error() << "\n";
        return 1;
    }
    if ((report_b.warning_flags & MINI_IMAGE_WARNING_TOO_DARK) == 0u) {
        std::cerr << "expected dark image warning for image_b\n";
        return 1;
    }
#else
    if (result.code != MINI_ERROR_NOT_IMPLEMENTED) {
        std::cerr << "expected MINI_ERROR_NOT_IMPLEMENTED when OpenCV is unavailable\n";
        return 1;
    }
#endif

    MiniImageId image_corrupt = 0;
    result = mini_import_capture_image(reopened, source_corrupt.string().c_str(), 2, 30.0, &image_corrupt);
    if (result.code != MINI_OK || image_corrupt != image_b + 1) {
        std::cerr << "mini_import_capture_image #3 failed: " << mini_get_last_error() << "\n";
        return 1;
    }

    MiniImageQualityReport report_corrupt{};
    result = mini_analyse_image_quality(reopened, image_corrupt, &report_corrupt);
#if defined(MINI_PAINTER_HAS_OPENCV)
    if (result.code != MINI_ERROR_IMAGE_DECODE) {
        std::cerr << "expected MINI_ERROR_IMAGE_DECODE for corrupt image but received: " << result.code << "\n";
        return 1;
    }
#else
    if (result.code != MINI_ERROR_NOT_IMPLEMENTED) {
        std::cerr << "expected MINI_ERROR_NOT_IMPLEMENTED when OpenCV is unavailable\n";
        return 1;
    }
#endif

    mini_close_project(reopened);

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
#if defined(MINI_PAINTER_HAS_OPENCV)
    if (json_text.find("\"processed_image_path\": \"processed/frame_001.png\"") == std::string::npos) {
        std::cerr << "project.json does not contain expected processed image metadata\n";
        return 1;
    }
#endif

    fs::remove_all(root, ec);

    std::cout << "mini_painter_tests passed\n";
    return 0;
}
