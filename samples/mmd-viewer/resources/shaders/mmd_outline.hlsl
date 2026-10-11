#include "material.hlsli"

float3 get_smooth_normal(mesh_vertex vertex, float3 smooth_normal)
{
    float3 n = normalize(vertex.normal);
    float3 t = normalize(vertex.tangent.xyz);
    float3 b = normalize(cross(n, t) * vertex.tangent.w);
    float3x3 tbn = transpose(float3x3(t, b, n));

    return normalize(mul(tbn, smooth_normal));
}
struct mmd_outline_material
{
    struct varying
    {
        float4 position_cs : SV_POSITION;
        float3 color : COLOR;
    };

    float3 color;
    float width;
    float z_offset;
    float strength;

    varying evaluate_varying(material_context context, mesh mesh)
    {
        float4 smooth_normal_and_outline = mesh.fetch_attribute<float4>(GEOMETRY_ATTRIBUTE_CUSTOM0);
        float3 smooth_normal_ws = mul((float3x3)mesh.get_model_matrix(), get_smooth_normal(mesh.vertex, smooth_normal_and_outline.xyz));

        float4 position_ws = mul(mesh.get_model_matrix(), float4(mesh.vertex.position, 1.0));
        float4 position_vs = mul(context.camera.matrix_v, position_ws);

        // https://github.com/ColinLeung-NiloCat/UnityURPToonLitShaderExample/blob/master/NiloOutlineUtil.hlsl
        float camera_mul_fix = abs(position_vs.z);
        camera_mul_fix = saturate(camera_mul_fix);
        camera_mul_fix *= context.camera.perspective_fov / PI * 180.0;
        camera_mul_fix *= 0.001;
        position_ws.xyz += smooth_normal_ws * width * camera_mul_fix;

        // z offset.
        position_vs = mul(context.camera.matrix_v, position_ws);
        position_vs.xyz += normalize(position_vs.xyz) * z_offset;

        varying varying;
        varying.position_cs = mul(context.camera.matrix_p, position_vs);
        varying.color = color * strength;

        return varying;
    }

    surface evaluate_surface(material_context context, varying varying)
    {
        surface surface;
        surface.albedo = varying.color;
        surface.roughness = 0.0;
        surface.metallic = 0.0;
        surface.emissive = 0.0;
        surface.normal_ws = 0.0;
        return surface;
    }
};