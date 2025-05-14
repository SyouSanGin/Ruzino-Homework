#include "material.h"

#include <pxr/imaging/hd/material.h>
#include <pxr/imaging/hd/materialNetwork2Interface.h>
#include <pxr/imaging/hio/image.h>

#include "nvrhi/nvrhi.h"
#include "pxr/base/arch/fileSystem.h"
#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/sdr/registry.h"
USTC_CG_NAMESPACE_OPEN_SCOPE

std::mutex Hd_USTC_CG_Material::texture_mutex;
std::mutex Hd_USTC_CG_Material::material_data_handle_mutex;

Hd_USTC_CG_Material::Hd_USTC_CG_Material(SdfPath const& id) : HdMaterial(id)
{
}

HdMaterialNetwork2Interface Hd_USTC_CG_Material::FetchNetInterface(
    HdSceneDelegate* sceneDelegate,
    HdMaterialNetwork2& hdNetwork,
    SdfPath& materialPath)
{
    VtValue material = sceneDelegate->GetMaterialResource(GetId());
    HdMaterialNetworkMap networkMap = material.Get<HdMaterialNetworkMap>();

    bool isVolume;
    hdNetwork = HdConvertToHdMaterialNetwork2(networkMap, &isVolume);

    materialPath = GetId();

    HdMaterialNetwork2Interface netInterface =
        HdMaterialNetwork2Interface(materialPath, &hdNetwork);
    return netInterface;
}

HdDirtyBits Hd_USTC_CG_Material::GetInitialDirtyBitsMask() const
{
    return HdChangeTracker::AllDirty;
}

void Hd_USTC_CG_Material::Finalize(HdRenderParam* renderParam)
{
    HdMaterial::Finalize(renderParam);
}

void Hd_USTC_CG_Material::ensure_material_data_handle(
    Hd_USTC_CG_RenderParam* render_param)
{
    std::lock_guard<std::mutex> lock(material_data_handle_mutex);
    if (!material_data_handle) {
        if (!render_param) {
            throw std::runtime_error("Render param is null.");
        }

        material_header_handle =
            render_param->InstanceCollection->material_header_pool.allocate(1);

        material_data_handle =
            render_param->InstanceCollection->material_pool.allocate(1);

        MaterialHeader header;
        header.material_blob_id = material_data_handle->index();
        header.material_type_id = material_header_handle->index();
        material_header_handle->write_data(&header);
    }
}

unsigned Hd_USTC_CG_Material::GetMaterialLocation() const
{
    if (!material_data_handle) {
        return -1;
    }
    return material_header_handle->index();
}

// HLSL callable shader
std::string Hd_USTC_CG_Material::slang_source_code_template = R"(
import Scene.VertexInfo;
import Scene.BindlessMaterial;
import utils.Math.ShadingFrame;

struct CallableData
{
    float4 color;
    float3 throughput;
    float3 L;
    float3 V;
    float3 sampledDir;
    float pdf;
    uint seed;
    uint materialBlobID;
    VertexInfo vertexInfo;
};

[shader("callable")]
void $getColor(inout CallableData data)
{
    MaterialDataBlob blob_data = materialBlobBuffer[data.materialBlobID];

    if (length(data.L) > 0.1)
        eval(data.color, data.L, data.V, blob_data, data.vertexInfo);
    data.sampledDir = sample(data.seed, data.pdf, blob_data);
    bool valid;
    ShadingFrame sf = ShadingFrame.createSafe(data.vertexInfo.normalW, float4(0, 0, 0, 1), valid);
    float3 world_ray_dir = sf.fromLocal(data.sampledDir);

    float4 throughput4;
    eval(throughput4 , world_ray_dir, data.V, blob_data, data.vertexInfo);
    data.throughput = throughput4.rgb;
}

)";

// HLSL callable shader
std::string Hd_USTC_CG_Material::eval_source_code_fallback = R"(

void eval(
    out float4 color,
    float3 L,
    float3 V,
    in MaterialDataBlob blob_data,
    VertexInfo vertexInfo)
{
    color = float4(0.8, 0.8, 0.8, 1);
}

)";

std::string Hd_USTC_CG_Material::sample_source_code_fallback = R"(
import Utils.Math.MathHelpers;
#include "utils/random.slangh"


float3 sample(
    inout uint seed,
    out float pdf,
    in MaterialDataBlob blob_data)
{
    // Sample the direction
    float3 sampledDir = sample_cosine_hemisphere_concentric(random_float2(seed), pdf);
    return sampledDir;
}

)";

void Hd_USTC_CG_Material::ensure_shader_ready(const ShaderFactory& factory)
{
    // Use fallback shader if no source is available
    if (material_name.empty()) {
        material_name = "fallback";
    }

    slang_source_code_main = slang_source_code_template;

    // Replace the callable function name with the material name in all code
    constexpr char FUNC_PLACEHOLDER[] = "$getColor";

    // Replace in local_slang_source_code
    auto pos = slang_source_code_main.find(FUNC_PLACEHOLDER);
    if (pos != std::string::npos) {
        slang_source_code_main.replace(
            pos, strlen(FUNC_PLACEHOLDER), material_name);
    }

    final_shader_source = eval_source_code_fallback +
                          sample_source_code_fallback + slang_source_code_main;
}

std::string Hd_USTC_CG_Material::GetShader(const ShaderFactory& factory)
{
    ensure_shader_ready(factory);
    return final_shader_source;
}

USTC_CG_NAMESPACE_CLOSE_SCOPE
