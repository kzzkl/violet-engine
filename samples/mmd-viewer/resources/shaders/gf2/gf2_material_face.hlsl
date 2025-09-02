#include "gf2/gf2_material.hlsli"
#include "shading/shading_model.hlsli"

struct gf2_material_face
{
    uint diffuse_texture;
    uint sdf_texture;
    uint ramp_texture;
    uint brdf_lut;
    float3 face_front_dir;
    uint padding0;
    float3 face_left_dir;
    uint padding1;
};

fs_output fs_main(vs_output input)
{
    SamplerState linear_repeat_sampler = get_linear_repeat_sampler();
    SamplerState linear_clamp_sampler = get_linear_clamp_sampler();
    
    gf2_material_face material = load_material<gf2_material_face>(scene.material_buffer, input.material_address);

    Texture2D<float4> diffuse_texture = ResourceDescriptorHeap[material.diffuse_texture];
    Texture2D<float4> sdf_texture = ResourceDescriptorHeap[material.sdf_texture];
    Texture2D<float4> ramp_texture = ResourceDescriptorHeap[material.ramp_texture];
    
    float3 albedo = diffuse_texture.Sample(linear_repeat_sampler, input.texcoord).rgb;
    float roughness = 1.0;
    float metallic = 0.0;

    float3 V = normalize(camera.position - input.position_ws);
    float3 N = normalize(input.normal_ws);

    StructuredBuffer<light_data> lights = ResourceDescriptorHeap[scene.light_buffer];

    float NdotV = saturate(dot(N, V));
    float3 F0 = lerp(0.04, albedo, metallic);

    float3 direct_lighting = 0.0;
    if (scene.light_count > 0)
    {
        light_data light = lights[0];

        float3 L = -light.direction;
        float3 H = normalize(L + V);

        float NdotL = 1.0;
        float NdotH = 1.0;
        float VdotH = 1.0;

        float d = d_ggx(NdotH, roughness);
        float vis = v_smith_joint_approx(NdotV, NdotL, roughness);
        float3 f = f_schlick(VdotH, F0);
        float3 kd = lerp(1.0 - f, 0.0, metallic);

        float3 specular = d * vis * f;
        float3 diffuse = albedo / PI * kd;

        float3 diffuse_ramp = ramp_texture.Sample(linear_clamp_sampler, float2(NdotL, 0.875)).rgb;
        float3 specular_ramp = ramp_texture.Sample(linear_clamp_sampler, float2(NdotL, 0.625)).rgb;

        float2 projected_light = normalize(L.xz);
        float2 front_dir = normalize(material.face_front_dir.xz);
        float2 left_dir = normalize(material.face_left_dir.xz);
        float threshold = dot(front_dir, normalize(L.xz));
        threshold = 1.0 - (threshold * 0.5 + 0.5);

        float sign = dot(left_dir, projected_light) > 0.0 ? 1.0 : -1.0;
        float4 sdf = sdf_texture.Sample(linear_repeat_sampler, float2(input.texcoord2.x * sign, input.texcoord2.y));
        float shadow = step(threshold, sdf.r);

        direct_lighting += (specular * specular_ramp + diffuse * diffuse_ramp) * light.color * shadow;
    }

    float3 indirect_lighting = indirect_light(material.face_front_dir, V, albedo, roughness, metallic, material.brdf_lut);

    material_info material_info = load_material_info(scene.material_buffer, input.material_address);

    fs_output output;
    output.albedo = float4(direct_lighting + indirect_lighting, 1.0);
    output.material = 0.0;
    output.normal = pack_gbuffer_normal(float3(0.0, 0.0, 0.0), material_info.shading_model);
    output.emissive = 0.0;

    return output;
}