#pragma once

#include <string>

#include "mini_painter/mini_painter.h"

namespace mini {

struct ProjectMetadata {
    std::string project_id;
    std::string display_name;
    std::string created_at;
    std::string updated_at;
};

class ProjectStore {
public:
    ProjectStore(std::string root_dir, std::string project_dir, ProjectMetadata metadata);

    static MiniResult create(const std::string& root_dir, const std::string& display_name, ProjectStore** out_store, std::string* out_error);
    static MiniResult open(const std::string& project_dir, ProjectStore** out_store, std::string* out_error);
    MiniResult save(std::string* out_error) const;

    const std::string& project_dir() const { return project_dir_; }

private:
    std::string root_dir_;
    std::string project_dir_;
    ProjectMetadata metadata_;
};

std::string utc_now_iso8601();
std::string generate_id();

}  // namespace mini
