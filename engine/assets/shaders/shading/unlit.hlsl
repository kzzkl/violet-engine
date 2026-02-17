#include "common.hlsli"
#include "shading/shading_model.hlsli"

struct constant_data
{
    constant_common common;
};
PushConstant(constant_data, constant);

ConstantBuffer<scene_data> scene : register(b0, space1);
ConstantBuffer<camera_data> camera : register(b0, space2);

struct unlit_shading_model
{
    float3 albedo;

    static unlit_shading_model create(gbuffer gbuffer, uint2 coord)
    {
        unlit_shading_model shading_model;
        shading_model.albedo = gbuffer.albedo;
        return shading_model;
    }

    float3 evaluate_direct_lighting(light_data light, float shadow_mask)
    {
        return 0.0;
    }

    float3 evaluate_indirect_lighting()
    {
        return albedo;
    }
};

[numthreads(SHADING_TILE_SIZE, SHADING_TILE_SIZE, 1)]
void cs_main(uint3 gtid : SV_GroupThreadID, uint3 gid : SV_GroupID)
{
    evaluate_lighting<unlit_shading_model>(constant.common, scene, camera, gtid, gid);
}