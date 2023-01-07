#pragma once

#include "ecs/entity.hpp"
#include "graphics_interface.hpp"
#include "math/math.hpp"
#include "mmd_bezier.hpp"
#include <memory>
#include <string>
#include <vector>

namespace violet::sample::mmd
{
struct mmd_node
{
    std::string name;
    std::uint32_t index;
    std::int32_t layer;

    bool deform_after_physics;

    bool inherit_local_flag;
    bool is_inherit_rotation;
    bool is_inherit_translation;

    ecs::entity inherit_node{ecs::INVALID_ENTITY};
    float inherit_weight;
    math::float3 inherit_translation{0.0f, 0.0f, 0.0f};
    math::float4 inherit_rotation{0.0f, 0.0f, 0.0f, 1.0f};

    math::float3 initial_position{0.0f, 0.0f, 0.0f};
    math::float4 initial_rotation{0.0f, 0.0f, 0.0f, 1.0f};
    math::float3 initial_scale{1.0f, 1.0f, 1.0f};
    math::float4x4 initial_inverse;
};

struct mmd_node_animation
{
    struct key
    {
        std::int32_t frame;
        math::float3 translate;
        math::float4 rotate;

        mmd_bezier tx_bezier;
        mmd_bezier ty_bezier;
        mmd_bezier tz_bezier;
        mmd_bezier r_bezier;
    };

    std::size_t offset;
    std::vector<key> keys;

    math::float3 translation{0.0f, 0.0f, 0.0f};
    math::float4 rotation{0.0f, 0.0f, 0.0f, 1.0f};
    math::float3 base_translation{0.0f, 0.0f, 0.0f};
    math::float4 base_rotation{0.0f, 0.0f, 0.0f, 1.0f};
};

struct mmd_ik_solver
{
    struct key
    {
        std::int32_t frame;
        bool enable;
    };

    bool enable{true};
    float limit_angle;

    std::size_t offset;
    std::vector<key> keys;
    bool base_animation;

    ecs::entity ik_target;
    std::vector<ecs::entity> links;

    std::uint32_t loop_count;
};

struct mmd_ik_link
{
    math::float4 ik_rotate{0.0f, 0.0f, 0.0f, 1.0f};

    bool enable_limit;
    math::float3 limit_max;
    math::float3 limit_min;
    math::float3 prev_angle;
    math::float4 save_ik_rotate;
    float plane_mode_angle;
};

struct mmd_skeleton
{
    std::vector<ecs::entity> nodes;
    std::vector<ecs::entity> sorted_nodes;

    std::vector<math::float4x4> local;
    std::vector<math::float4x4> world;
};

struct mmd_morph_controler
{
    struct key
    {
        std::int32_t frame;
        float weight;
    };

    class morph
    {
    public:
        virtual ~morph() = default;

        virtual void evaluate(float weight, ecs::entity entity) = 0;

        std::vector<key> keys;
        std::size_t offset;
    };

    class group_morph : public morph
    {
    public:
        struct data_type
        {
            std::int32_t index;
            float weight;
        };

    public:
        virtual void evaluate(float weight, ecs::entity entity) override;

        std::vector<data_type> data;
    };

    class vertex_morph : public morph
    {
    public:
        struct data_type
        {
            std::int32_t index;
            math::float3 translation;
        };

    public:
        virtual void evaluate(float weight, ecs::entity entity) override;

        std::vector<data_type> data;
    };

    class bone_morph : public morph
    {
    public:
        struct data_type
        {
            std::int32_t index;
            math::float3 translation;
            math::float4 rotation;
        };

    public:
        virtual void evaluate(float weight, ecs::entity entity) override;

        std::vector<data_type> data;
    };

    class uv_morph : public morph
    {
    public:
        struct data_type
        {
            std::int32_t index;
            math::float4 uv;
        };

    public:
        virtual void evaluate(float weight, ecs::entity entity) override;

        std::vector<data_type> data;
    };

    class material_morph : public morph
    {
    public:
        struct data_type
        {
            std::int32_t index;
            std::uint8_t operate; // 0: mul, 1: add
            math::float4 diffuse;
            math::float3 specular;
            float specular_strength;
            math::float3 ambient;
            math::float4 edge_color;
            float edge_scale;

            math::float4 tex_tint;
            math::float4 spa_tint;
            math::float4 toon_tint;
        };

    public:
        virtual void evaluate(float weight, ecs::entity entity) override;

        std::vector<data_type> data;
    };

    class flip_morph : public morph
    {
    public:
        virtual void evaluate(float weight, ecs::entity entity) override {}
    };

    class impulse_morph : public morph
    {
    public:
        virtual void evaluate(float weight, ecs::entity entity) override {}
    };

    std::vector<std::unique_ptr<morph>> morphs;

    std::unique_ptr<graphics::resource_interface> vertex_morph_result;
    std::unique_ptr<graphics::resource_interface> uv_morph_result;
};
} // namespace violet::sample::mmd