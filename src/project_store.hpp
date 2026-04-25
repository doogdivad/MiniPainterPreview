#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "mini_painter/mini_painter.h"

namespace mini {

struct ProjectMetadata {
    std::string project_id;
    std::string display_name;
    std::string created_at;
    std::string updated_at;

    struct CaptureImage {
        uint64_t image_id;
        std::string file_path;
        int angle_index;
        double estimated_angle_degrees;
        double quality_score;
        std::string mask_path;
        std::string processed_image_path;
    };

    std::vector<CaptureImage> capture_set;
};

class ProjectStore {
public:
    ProjectStore(std::string root_dir, std::string project_dir, ProjectMetadata metadata);

    static MiniResult create(const std::string& root_dir, const std::string& display_name, ProjectStore** out_store, std::string* out_error);
    static MiniResult open(const std::string& project_dir, ProjectStore** out_store, std::string* out_error);
    MiniResult save(std::string* out_error) const;
    MiniResult import_capture_image(const std::string& source_image_path, int angle_index, double estimated_angle_degrees, uint64_t* out_image_id, std::string* out_error);
    MiniResult set_image_quality_score(uint64_t image_id, double quality_score, std::string* out_error);
    MiniResult set_image_mask_path(uint64_t image_id, const std::string& mask_path, std::string* out_error);

    const std::string& project_dir() const { return project_dir_; }
    const ProjectMetadata& metadata() const { return metadata_; }

private:
    std::string root_dir_;
    std::string project_dir_;
    ProjectMetadata metadata_;
};

std::string utc_now_iso8601();
std::string generate_id();

}  // namespace mini
