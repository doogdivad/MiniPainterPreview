#include "mini_painter/mini_painter.h"

#include <algorithm>
#include <filesystem>
#include <memory>
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

MiniResult mini_set_external_mask(MiniProjectHandle, MiniImageId, const char*) {
    return not_implemented("mini_set_external_mask");
}

MiniResult mini_cleanup_mask(MiniProjectHandle, MiniImageId, MiniMaskCleanupOptions) {
    return not_implemented("mini_cleanup_mask");
}

MiniResult mini_generate_processed_frame(MiniProjectHandle, MiniImageId, MiniFrameOptions) {
    return not_implemented("mini_generate_processed_frame");
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
