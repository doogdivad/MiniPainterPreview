#include "mini_painter/mini_painter.h"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>

#if defined(MINI_PAINTER_HAS_OPENCV)
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#endif

#include "project_store.hpp"

struct MiniProjectOpaque {
    std::unique_ptr<mini::ProjectStore> store;
};

namespace {
thread_local std::string g_last_error;

MiniResult not_implemented(const char* fn) {
    g_last_error = std::string(fn) + " is not implemented yet";
    return {MINI_ERROR_NOT_IMPLEMENTED};
}

MiniResult fail(MiniResultCode code, const std::string& message) {
    g_last_error = message;
    return {code};
}

MiniResult success() {
    g_last_error.clear();
    return {MINI_OK};
}
}  // namespace

MiniResult mini_create_project(const char* root_dir, const char* display_name, MiniProjectHandle* out_project) {
    if (root_dir == nullptr || display_name == nullptr || out_project == nullptr) {
        return fail(MINI_ERROR_INVALID_ARGUMENT, "mini_create_project received null argument");
    }

    mini::ProjectStore* raw = nullptr;
    std::string error;
    MiniResult result = mini::ProjectStore::create(root_dir, display_name, &raw, &error);
    if (result.code != MINI_OK) {
        return fail(result.code, error.empty() ? "mini_create_project failed" : error);
    }

    auto* opaque = new MiniProjectOpaque();
    opaque->store.reset(raw);
    *out_project = opaque;
    return success();
}

MiniResult mini_open_project(const char* project_path, MiniProjectHandle* out_project) {
    if (project_path == nullptr || out_project == nullptr) {
        return fail(MINI_ERROR_INVALID_ARGUMENT, "mini_open_project received null argument");
    }

    mini::ProjectStore* raw = nullptr;
    std::string error;
    MiniResult result = mini::ProjectStore::open(project_path, &raw, &error);
    if (result.code != MINI_OK) {
        return fail(result.code, error.empty() ? "mini_open_project failed" : error);
    }

    auto* opaque = new MiniProjectOpaque();
    opaque->store.reset(raw);
    *out_project = opaque;
    return success();
}

MiniResult mini_save_project(MiniProjectHandle project) {
    if (project == nullptr) {
        return fail(MINI_ERROR_INVALID_ARGUMENT, "mini_save_project received null project handle");
    }

    std::string error;
    MiniResult result = project->store->save(&error);
    if (result.code != MINI_OK) {
        return fail(result.code, error.empty() ? "mini_save_project failed" : error);
    }

    return success();
}

void mini_close_project(MiniProjectHandle project) {
    delete project;
}

MiniResult mini_import_capture_image(MiniProjectHandle project, const char* source_image_path, int angle_index, double estimated_angle_degrees, MiniImageId* out_image_id) {
    if (project == nullptr || source_image_path == nullptr || out_image_id == nullptr) {
        return fail(MINI_ERROR_INVALID_ARGUMENT, "mini_import_capture_image received null argument");
    }

    std::string error;
    MiniResult result = project->store->import_capture_image(source_image_path, angle_index, estimated_angle_degrees, out_image_id, &error);
    if (result.code != MINI_OK) {
        return fail(result.code, error.empty() ? "mini_import_capture_image failed" : error);
    }
    return success();
}

MiniResult mini_analyse_image_quality(MiniProjectHandle project, MiniImageId image_id, MiniImageQualityReport* out_report) {
#if !defined(MINI_PAINTER_HAS_OPENCV)
    (void) project;
    (void) image_id;
    (void) out_report;
    return not_implemented("mini_analyse_image_quality (OpenCV not available)");
#else
    if (project == nullptr || image_id == 0 || out_report == nullptr) {
        return fail(MINI_ERROR_INVALID_ARGUMENT, "mini_analyse_image_quality received invalid argument");
    }

    const auto& capture_set = project->store->metadata().capture_set;
    auto image_it = std::find_if(capture_set.begin(), capture_set.end(), [image_id](const mini::ProjectMetadata::CaptureImage& image) {
        return image.image_id == image_id;
    });
    if (image_it == capture_set.end()) {
        return fail(MINI_ERROR_INVALID_ARGUMENT, "image id not found in capture_set");
    }

    const std::filesystem::path image_path = std::filesystem::path(project->store->project_dir()) / image_it->file_path;
    const cv::Mat image = cv::imread(image_path.string(), cv::IMREAD_COLOR);
    if (image.empty()) {
        return fail(MINI_ERROR_IMAGE_DECODE, "failed to decode image from capture path");
    }

    cv::Mat gray;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);

    cv::Mat laplacian;
    cv::Laplacian(gray, laplacian, CV_64F);
    cv::Scalar mean{};
    cv::Scalar stddev{};
    cv::meanStdDev(laplacian, mean, stddev);
    const double blur_score = stddev[0] * stddev[0];

    const double exposure_score = cv::mean(gray)[0] / 255.0;

    cv::Mat otsu_binary;
    cv::threshold(gray, otsu_binary, 0.0, 255.0, cv::THRESH_BINARY | cv::THRESH_OTSU);
    const int white_pixels = cv::countNonZero(otsu_binary);
    const int total_pixels = gray.rows * gray.cols;
    const int foreground_pixels = std::min(white_pixels, total_pixels - white_pixels);
    const double subject_coverage_estimate = total_pixels > 0
                                                 ? static_cast<double>(foreground_pixels) / static_cast<double>(total_pixels)
                                                 : 0.0;

    constexpr double kBlurThreshold = 120.0;
    constexpr double kDarkThreshold = 0.25;
    constexpr double kBrightThreshold = 0.85;
    constexpr double kLikelyCroppedCoverageThreshold = 0.92;
    constexpr int kMinimumSupportedDimension = 512;

    uint32_t warning_flags = 0;
    if (blur_score < kBlurThreshold) {
        warning_flags |= MINI_IMAGE_WARNING_TOO_BLURRY;
    }
    if (exposure_score < kDarkThreshold) {
        warning_flags |= MINI_IMAGE_WARNING_TOO_DARK;
    }
    if (exposure_score > kBrightThreshold) {
        warning_flags |= MINI_IMAGE_WARNING_TOO_BRIGHT;
    }
    if (subject_coverage_estimate > kLikelyCroppedCoverageThreshold) {
        warning_flags |= MINI_IMAGE_WARNING_LIKELY_CROPPED;
    }
    if (image.cols < kMinimumSupportedDimension || image.rows < kMinimumSupportedDimension) {
        warning_flags |= MINI_IMAGE_WARNING_LOW_RESOLUTION;
    }

    out_report->blur_score = blur_score;
    out_report->exposure_score = exposure_score;
    out_report->width = image.cols;
    out_report->height = image.rows;
    out_report->subject_coverage_estimate = subject_coverage_estimate;
    out_report->warning_flags = warning_flags;

    const int warning_count = __builtin_popcount(warning_flags);
    const double quality_score = 1.0 - (static_cast<double>(warning_count) / 5.0);
    std::string error;
    MiniResult quality_write_result = project->store->set_image_quality_score(image_id, quality_score, &error);
    if (quality_write_result.code != MINI_OK) {
        return fail(quality_write_result.code, error.empty() ? "failed to persist quality score" : error);
    }

    return success();
#endif
}

#if defined(MINI_PAINTER_HAS_OPENCV)
namespace {
bool write_binary_mask_png(const cv::Mat& mask, const std::filesystem::path& output_path, std::string* out_error) {
    cv::Mat normalized_mask;
    if (mask.type() == CV_8UC1) {
        normalized_mask = mask;
    } else {
        mask.convertTo(normalized_mask, CV_8UC1);
    }
    cv::threshold(normalized_mask, normalized_mask, 0, 255, cv::THRESH_BINARY);
    if (!cv::imwrite(output_path.string(), normalized_mask)) {
        *out_error = "failed to write mask png";
        return false;
    }
    return true;
}

MiniResult generate_fallback_mask(const cv::Mat& image, cv::Mat* out_mask, std::string* out_error) {
    if (out_mask == nullptr || out_error == nullptr) {
        return fail(MINI_ERROR_INVALID_ARGUMENT, "invalid output pointers for fallback mask generation");
    }

    cv::Mat gray;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    cv::Mat thresholded;
    cv::threshold(gray, thresholded, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
    const int white_pixels = cv::countNonZero(thresholded);
    const int total_pixels = thresholded.rows * thresholded.cols;
    if (white_pixels > (total_pixels / 2)) {
        cv::bitwise_not(thresholded, thresholded);
    }

    cv::Mat morph_kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
    cv::morphologyEx(thresholded, *out_mask, cv::MORPH_OPEN, morph_kernel);
    cv::morphologyEx(*out_mask, *out_mask, cv::MORPH_CLOSE, morph_kernel);
    return success();
}

MiniResult resolve_image_and_mask_paths(MiniProjectHandle project, MiniImageId image_id, std::filesystem::path* out_image_path, std::filesystem::path* out_mask_path) {
    const auto& capture_set = project->store->metadata().capture_set;
    auto image_it = std::find_if(capture_set.begin(), capture_set.end(), [image_id](const mini::ProjectMetadata::CaptureImage& image) {
        return image.image_id == image_id;
    });
    if (image_it == capture_set.end()) {
        return fail(MINI_ERROR_INVALID_ARGUMENT, "image id not found in capture_set");
    }

    std::ostringstream mask_name;
    mask_name << "mask_" << image_id << ".png";

    if (out_image_path != nullptr) {
        *out_image_path = std::filesystem::path(project->store->project_dir()) / image_it->file_path;
    }
    if (out_mask_path != nullptr) {
        *out_mask_path = std::filesystem::path(project->store->project_dir()) / "masks" / mask_name.str();
    }
    return success();
}

MiniResult resolve_processed_frame_path(MiniProjectHandle project, MiniImageId image_id, std::filesystem::path* out_processed_path) {
    if (project == nullptr || image_id == 0 || out_processed_path == nullptr) {
        return fail(MINI_ERROR_INVALID_ARGUMENT, "invalid arguments resolving processed frame path");
    }
    std::ostringstream filename;
    filename << "frame_" << std::setfill('0') << std::setw(3) << image_id << ".png";
    *out_processed_path = std::filesystem::path(project->store->project_dir()) / "processed" / filename.str();
    return success();
}
}  // namespace
#endif

MiniResult mini_set_external_mask(MiniProjectHandle project, MiniImageId image_id, const char* mask_png_path) {
#if !defined(MINI_PAINTER_HAS_OPENCV)
    (void) project;
    (void) image_id;
    (void) mask_png_path;
    return not_implemented("mini_set_external_mask (OpenCV not available)");
#else
    if (project == nullptr || image_id == 0 || mask_png_path == nullptr) {
        return fail(MINI_ERROR_INVALID_ARGUMENT, "mini_set_external_mask received invalid argument");
    }

    const std::filesystem::path source_path(mask_png_path);
    if (!std::filesystem::exists(source_path)) {
        return fail(MINI_ERROR_FILE_NOT_FOUND, "mask png path does not exist");
    }

    std::filesystem::path image_path;
    std::filesystem::path output_mask_path;
    MiniResult resolve_result = resolve_image_and_mask_paths(project, image_id, &image_path, &output_mask_path);
    if (resolve_result.code != MINI_OK) {
        return resolve_result;
    }

    cv::Mat source_mask = cv::imread(source_path.string(), cv::IMREAD_GRAYSCALE);
    if (source_mask.empty()) {
        return fail(MINI_ERROR_IMAGE_DECODE, "failed to decode external mask png");
    }

    cv::Mat source_image = cv::imread(image_path.string(), cv::IMREAD_COLOR);
    if (source_image.empty()) {
        return fail(MINI_ERROR_IMAGE_DECODE, "failed to decode source capture image");
    }

    if (source_mask.size() != source_image.size()) {
        cv::resize(source_mask, source_mask, source_image.size(), 0, 0, cv::INTER_NEAREST);
    }

    std::string write_error;
    if (!write_binary_mask_png(source_mask, output_mask_path, &write_error)) {
        return fail(MINI_ERROR_IO, write_error);
    }

    std::string save_error;
    const std::filesystem::path rel_mask = std::filesystem::path("masks") / output_mask_path.filename();
    MiniResult save_result = project->store->set_image_mask_path(image_id, rel_mask.generic_string(), &save_error);
    if (save_result.code != MINI_OK) {
        return fail(save_result.code, save_error.empty() ? "failed to persist image mask path" : save_error);
    }

    return success();
#endif
}

MiniResult mini_cleanup_mask(MiniProjectHandle project, MiniImageId image_id, MiniMaskCleanupOptions options) {
#if !defined(MINI_PAINTER_HAS_OPENCV)
    (void) project;
    (void) image_id;
    (void) options;
    return not_implemented("mini_cleanup_mask (OpenCV not available)");
#else
    if (project == nullptr || image_id == 0) {
        return fail(MINI_ERROR_INVALID_ARGUMENT, "mini_cleanup_mask received invalid argument");
    }

    std::filesystem::path image_path;
    std::filesystem::path output_mask_path;
    MiniResult resolve_result = resolve_image_and_mask_paths(project, image_id, &image_path, &output_mask_path);
    if (resolve_result.code != MINI_OK) {
        return resolve_result;
    }

    cv::Mat mask = cv::imread(output_mask_path.string(), cv::IMREAD_GRAYSCALE);
    if (mask.empty()) {
        cv::Mat source_image = cv::imread(image_path.string(), cv::IMREAD_COLOR);
        if (source_image.empty()) {
            return fail(MINI_ERROR_IMAGE_DECODE, "failed to decode source image for fallback mask");
        }
        std::string fallback_error;
        MiniResult fallback_result = generate_fallback_mask(source_image, &mask, &fallback_error);
        if (fallback_result.code != MINI_OK) {
            return fail(fallback_result.code, fallback_error.empty() ? "fallback mask generation failed" : fallback_error);
        }
    }

    cv::threshold(mask, mask, 0, 255, cv::THRESH_BINARY);

    if (options.remove_small_islands > 0) {
        cv::Mat labels;
        cv::Mat stats;
        cv::Mat centroids;
        const int num_labels = cv::connectedComponentsWithStats(mask, labels, stats, centroids, 8, CV_32S);
        cv::Mat cleaned = cv::Mat::zeros(mask.size(), CV_8UC1);
        for (int label = 1; label < num_labels; ++label) {
            const int area = stats.at<int>(label, cv::CC_STAT_AREA);
            if (area >= options.remove_small_islands) {
                cleaned.setTo(255, labels == label);
            }
        }
        mask = cleaned;
    }

    if (options.fill_holes > 0) {
        cv::Mat flood = mask.clone();
        cv::floodFill(flood, cv::Point(0, 0), cv::Scalar(255));
        cv::Mat flood_inv;
        cv::bitwise_not(flood, flood_inv);
        mask |= flood_inv;
    }

    if (options.dilate_erode_amount != 0) {
        const int kernel_size = std::max(1, std::abs(options.dilate_erode_amount) * 2 + 1);
        cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(kernel_size, kernel_size));
        if (options.dilate_erode_amount > 0) {
            cv::dilate(mask, mask, kernel);
        } else {
            cv::erode(mask, mask, kernel);
        }
    }

    if (options.feather_edge > 0) {
        const int blur_size = std::max(1, options.feather_edge * 2 + 1);
        cv::GaussianBlur(mask, mask, cv::Size(blur_size, blur_size), 0.0);
        cv::threshold(mask, mask, 127, 255, cv::THRESH_BINARY);
    }

    std::string write_error;
    if (!write_binary_mask_png(mask, output_mask_path, &write_error)) {
        return fail(MINI_ERROR_IO, write_error);
    }

    std::string save_error;
    const std::filesystem::path rel_mask = std::filesystem::path("masks") / output_mask_path.filename();
    MiniResult save_result = project->store->set_image_mask_path(image_id, rel_mask.generic_string(), &save_error);
    if (save_result.code != MINI_OK) {
        return fail(save_result.code, save_error.empty() ? "failed to persist cleaned mask path" : save_error);
    }

    return success();
#endif
}

MiniResult mini_generate_processed_frame(MiniProjectHandle project, MiniImageId image_id, MiniFrameOptions options) {
#if !defined(MINI_PAINTER_HAS_OPENCV)
    (void) project;
    (void) image_id;
    (void) options;
    return not_implemented("mini_generate_processed_frame (OpenCV not available)");
#else
    if (project == nullptr || image_id == 0) {
        return fail(MINI_ERROR_INVALID_ARGUMENT, "mini_generate_processed_frame received invalid argument");
    }

    std::filesystem::path image_path;
    std::filesystem::path output_mask_path;
    MiniResult resolve_result = resolve_image_and_mask_paths(project, image_id, &image_path, &output_mask_path);
    if (resolve_result.code != MINI_OK) {
        return resolve_result;
    }

    cv::Mat source_image = cv::imread(image_path.string(), cv::IMREAD_COLOR);
    if (source_image.empty()) {
        return fail(MINI_ERROR_IMAGE_DECODE, "failed to decode source image");
    }

    cv::Mat mask = cv::imread(output_mask_path.string(), cv::IMREAD_GRAYSCALE);
    if (mask.empty()) {
        std::string fallback_error;
        MiniResult fallback_result = generate_fallback_mask(source_image, &mask, &fallback_error);
        if (fallback_result.code != MINI_OK) {
            return fail(fallback_result.code, fallback_error.empty() ? "fallback mask generation failed" : fallback_error);
        }
    }
    if (mask.size() != source_image.size()) {
        cv::resize(mask, mask, source_image.size(), 0, 0, cv::INTER_NEAREST);
    }
    cv::threshold(mask, mask, 0, 255, cv::THRESH_BINARY);

    std::vector<cv::Point> foreground_points;
    cv::findNonZero(mask, foreground_points);
    if (foreground_points.empty()) {
        return fail(MINI_ERROR_PROCESSING, "mask contains no foreground pixels");
    }

    const cv::Rect subject_bounds = cv::boundingRect(foreground_points);
    cv::Mat cropped_bgr = source_image(subject_bounds).clone();
    cv::Mat cropped_alpha = mask(subject_bounds).clone();

    int output_width = subject_bounds.width;
    int output_height = subject_bounds.height;
    if (options.normalize_canvas > 0) {
        const int side = std::max(subject_bounds.width, subject_bounds.height);
        output_width = side;
        output_height = side;
    }

    cv::Mat output_rgba(output_height, output_width, CV_8UC4, cv::Scalar(0, 0, 0, 0));
    const int x_offset = (output_width - cropped_bgr.cols) / 2;
    const int y_offset = (output_height - cropped_bgr.rows) / 2;
    const cv::Rect output_roi(x_offset, y_offset, cropped_bgr.cols, cropped_bgr.rows);

    cv::Mat output_roi_rgba = output_rgba(output_roi);
    cv::Mat cropped_rgba;
    cv::cvtColor(cropped_bgr, cropped_rgba, cv::COLOR_BGR2BGRA);
    std::vector<cv::Mat> channels;
    cv::split(cropped_rgba, channels);
    channels[3] = cropped_alpha;
    cv::merge(channels, cropped_rgba);
    cropped_rgba.copyTo(output_roi_rgba);

    std::filesystem::path output_frame_path;
    MiniResult processed_path_result = resolve_processed_frame_path(project, image_id, &output_frame_path);
    if (processed_path_result.code != MINI_OK) {
        return processed_path_result;
    }

    if (!cv::imwrite(output_frame_path.string(), output_rgba)) {
        return fail(MINI_ERROR_IO, "failed to write processed frame png");
    }

    std::string persist_error;
    const std::filesystem::path rel_frame = std::filesystem::path("processed") / output_frame_path.filename();
    MiniResult persist_result = project->store->set_image_processed_path(image_id, rel_frame.generic_string(), &persist_error);
    if (persist_result.code != MINI_OK) {
        return fail(persist_result.code, persist_error.empty() ? "failed to persist processed frame path" : persist_error);
    }

    return success();
#endif
}

MiniResult mini_build_preview_asset(MiniProjectHandle, MiniPreviewBuildOptions) {
    return not_implemented("mini_build_preview_asset");
}

MiniResult mini_create_paint_scheme(MiniProjectHandle, const char*, MiniPaintSchemeId*) {
    return not_implemented("mini_create_paint_scheme");
}

MiniResult mini_add_paint_layer(MiniProjectHandle, MiniPaintSchemeId, const char*, const char*, MiniColourRGBA, float, MiniBlendMode) {
    return not_implemented("mini_add_paint_layer");
}

MiniResult mini_render_scheme_frame(MiniProjectHandle, MiniPaintSchemeId, int, const char*) {
    return not_implemented("mini_render_scheme_frame");
}

const char* mini_get_last_error() {
    return g_last_error.c_str();
}
