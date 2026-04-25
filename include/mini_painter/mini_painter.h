#ifndef MINI_PAINTER_MINI_PAINTER_H
#define MINI_PAINTER_MINI_PAINTER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MiniProjectOpaque* MiniProjectHandle;
typedef uint64_t MiniImageId;
typedef uint64_t MiniPaintSchemeId;
typedef uint64_t MiniLayerId;

typedef enum MiniResultCode {
    MINI_OK = 0,
    MINI_ERROR_INVALID_ARGUMENT,
    MINI_ERROR_FILE_NOT_FOUND,
    MINI_ERROR_IO,
    MINI_ERROR_IMAGE_DECODE,
    MINI_ERROR_PROCESSING,
    MINI_ERROR_PROJECT_CORRUPT,
    MINI_ERROR_NOT_IMPLEMENTED
} MiniResultCode;

typedef struct MiniResult {
    MiniResultCode code;
} MiniResult;

typedef enum MiniBlendMode {
    MINI_BLEND_NORMAL = 0,
    MINI_BLEND_MULTIPLY,
    MINI_BLEND_OVERLAY,
    MINI_BLEND_COLOUR
} MiniBlendMode;

typedef struct MiniColourRGBA {
    float r;
    float g;
    float b;
    float a;
} MiniColourRGBA;

typedef struct MiniMaskCleanupOptions {
    int remove_small_islands;
    int fill_holes;
    int feather_edge;
    int dilate_erode_amount;
} MiniMaskCleanupOptions;

typedef struct MiniFrameOptions {
    int normalize_canvas;
} MiniFrameOptions;

typedef struct MiniPreviewBuildOptions {
    int minimum_frames;
} MiniPreviewBuildOptions;

typedef struct MiniImageQualityReport {
    double blur_score;
    double exposure_score;
    int width;
    int height;
    double subject_coverage_estimate;
    uint32_t warning_flags;
} MiniImageQualityReport;

MiniResult mini_create_project(const char* root_dir, const char* display_name, MiniProjectHandle* out_project);
MiniResult mini_open_project(const char* project_path, MiniProjectHandle* out_project);
MiniResult mini_save_project(MiniProjectHandle project);
void mini_close_project(MiniProjectHandle project);

MiniResult mini_import_capture_image(MiniProjectHandle project, const char* source_image_path, int angle_index, double estimated_angle_degrees, MiniImageId* out_image_id);
MiniResult mini_analyse_image_quality(MiniProjectHandle project, MiniImageId image_id, MiniImageQualityReport* out_report);
MiniResult mini_set_external_mask(MiniProjectHandle project, MiniImageId image_id, const char* mask_png_path);
MiniResult mini_cleanup_mask(MiniProjectHandle project, MiniImageId image_id, MiniMaskCleanupOptions options);
MiniResult mini_generate_processed_frame(MiniProjectHandle project, MiniImageId image_id, MiniFrameOptions options);
MiniResult mini_build_preview_asset(MiniProjectHandle project, MiniPreviewBuildOptions options);

MiniResult mini_create_paint_scheme(MiniProjectHandle project, const char* name, MiniPaintSchemeId* out_scheme_id);
MiniResult mini_add_paint_layer(MiniProjectHandle project, MiniPaintSchemeId scheme_id, const char* name, const char* mask_png_path, MiniColourRGBA colour, float opacity, MiniBlendMode blend_mode);
MiniResult mini_render_scheme_frame(MiniProjectHandle project, MiniPaintSchemeId scheme_id, int frame_index, const char* output_png_path);

const char* mini_get_last_error();

#ifdef __cplusplus
}
#endif

#endif
