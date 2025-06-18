#pragma once
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/common.h>
#include <pxr/base/gf/vec3d.h>
#include <memory>

struct PickEvent {
    pxr::GfVec3d point;
    pxr::GfVec3d normal;
    pxr::SdfPath prim_path;
    pxr::SdfPath instancer_path;
    
    PickEvent(const pxr::GfVec3d& pt, const pxr::GfVec3d& norm, const pxr::SdfPath& path, const pxr::SdfPath& inst)
        : point(pt), normal(norm), prim_path(path), instancer_path(inst) {}
};

struct GeomPayload {
    pxr::UsdStageRefPtr stage;
    pxr::SdfPath prim_path;

    float delta_time = 0.0f;
    bool has_simulation = false;
    bool is_simulating = false;
    pxr::UsdTimeCode current_time = pxr::UsdTimeCode(0);

    std::string stage_filepath_;
    std::shared_ptr<PickEvent> pick;
};
