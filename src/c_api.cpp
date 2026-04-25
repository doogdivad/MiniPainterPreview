#include "mini_painter/mini_painter.h"

#include <memory>
#include <string>

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

MiniResult mini_analyse_image_quality(MiniProjectHandle, MiniImageId, MiniImageQualityReport*) {
    return not_implemented("mini_analyse_image_quality");
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
