#pragma once

#include "math/math.hpp"
#include <string>
#include <vector>

namespace violet::sample
{
enum pmx_vertex_type : std::uint8_t
{
    PMX_VERTEX_TYPE_BDEF1,
    PMX_VERTEX_TYPE_BDEF2,
    PMX_VERTEX_TYPE_BDEF4,
    PMX_VERTEX_TYPE_SDEF,
    PMX_VERTEX_TYPE_QDEF,
};

struct pmx_header
{
    float version;

    std::uint8_t text_encoding; // 1: UTF8, 0: UTF16
    std::uint8_t num_add_vec4;
    std::uint8_t vertex_index_size;
    std::uint8_t texture_index_size;
    std::uint8_t material_index_size;
    std::uint8_t bone_index_size;
    std::uint8_t morph_index_size;
    std::uint8_t rigidbody_index_size;

    std::string name_jp;
    std::string name_en;

    std::string comments_jp;
    std::string comments_en;
};

enum pmx_draw_flag : std::uint8_t
{
    PMX_DRAW_FLAG_NO_CULL = 0x01,
    PMX_DRAW_FLAG_GROUND_SHADOW = 0x02,
    PMX_DRAW_FLAG_DRAW_SHADOW = 0x04,
    PMX_DRAW_FLAG_RECEIVE_SHADOW = 0x08,
    PMX_DRAW_FLAG_HAS_EDGE = 0x10,
    PMX_DRAW_FLAG_VERTEX_COLOUR = 0x20,
    PMX_DRAW_FLAG_POINT_DRAWING = 0x40,
    PMX_DRAW_FLAG_LINE_DRAWING = 0x80,
};

enum pmx_sphere_mode : std::uint8_t
{
    PMX_SPHERE_MODE_DISABLED = 0,
    PMX_SPHERE_MODE_MULTIPLY = 1,
    PMX_SPHERE_MODE_ADDITIVE = 2,
    PMX_SPHERE_MODE_ADDITIONAL_VEC4 = 3
};

enum pmx_toon_mode : std::uint8_t
{
    PMX_TOON_MODE_TEXTURE,
    PMX_TOON_MODE_INTERNAL
};

struct pmx_material
{
    std::string name_jp;
    std::string name_en;

    float4 diffuse;
    float3 specular;
    float specular_strength;
    float3 ambient;
    pmx_draw_flag flag;
    float4 edge_color;
    float edge_size;

    std::int32_t texture_index;
    std::int32_t toon_index;
    std::int32_t sphere_index;
    pmx_sphere_mode sphere_mode;
    pmx_toon_mode toon_mode;

    std::string meta_data;
    std::int32_t index_count;
};

enum pmx_bone_flag : std::uint16_t
{
    PMX_BONE_FLAG_INDEXED_TAIL_POSITION = 0x0001,
    PMX_BONE_FLAG_ROTATABLE = 0x0002,
    PMX_BONE_FLAG_TRANSLATABLE = 0x0004,
    PMX_BONE_FLAG_VISIBLE = 0x0008,
    PMX_BONE_FLAG_ENABLED = 0x0010,
    PMX_BONE_FLAG_IK = 0x0020,
    PMX_BONE_FLAG_INHERIT_LOCAL = 0x0080,
    PMX_BONE_FLAG_INHERIT_ROTATION = 0x0100,
    PMX_BONE_FLAG_INHERIT_TRANSLATION = 0x0200,
    PMX_BONE_FLAG_FIXED_AXIS = 0x0400,
    PMX_BONE_FLAG_LOCAL_AXIS = 0x0800,
    PMX_BONE_FLAG_PHYSICS_AFTER_DEFORM = 0x1000,
    PMX_BONE_FLAG_EXTERNAL_PARENT_DEFORM = 0x2000
};

struct pmx_ik_link
{
    std::int32_t bone_index;
    bool enable_limit;
    float3 limit_min;
    float3 limit_max;
};

struct pmx_bone
{
    std::string name_jp;
    std::string name_en;

    float3 position;
    std::int32_t parent_index;
    std::int32_t layer;
    pmx_bone_flag flags;
    float3 tail_position;
    std::int32_t tail_index;
    std::int32_t inherit_index;
    float inherit_weight;
    float3 fixed_axis;
    float3 local_x_axis;
    float3 local_z_axis;
    std::int32_t external_parent_index;

    std::int32_t ik_target_index;
    std::int32_t ik_iteration_count;
    float ik_limit;
    std::vector<pmx_ik_link> ik_links;
};

enum pmx_morph_type : std::uint8_t
{
    PMX_MORPH_TYPE_GROUP,
    PMX_MORPH_TYPE_VERTEX,
    PMX_MORPH_TYPE_BONE,
    PMX_MORPH_TYPE_UV,
    PMX_MORPH_TYPE_UV_EXT_1,
    PMX_MORPH_TYPE_UV_EXT_2,
    PMX_MORPH_TYPE_UV_EXT_3,
    PMX_MORPH_TYPE_UV_EXT_4,
    PMX_MORPH_TYPE_MATERIAL,
    PMX_MORPH_TYPE_FLIP,
    PMX_MORPH_TYPE_IMPULSE
};

struct pmx_group_morph
{
    std::int32_t index;
    float weight;
};

struct pmx_vertex_morph
{
    std::int32_t index;
    float3 translation;
};

struct pmx_bone_morph
{
    std::int32_t index;
    float3 translation;
    float4 rotation;
};

struct pmx_uv_morph
{
    std::int32_t index;
    float4 uv;
};

struct pmx_material_morph
{
    std::int32_t index;
    std::uint8_t operate; // 0: mul, 1: add
    float4 diffuse;
    float3 specular;
    float specular_strength;
    float3 ambient;
    float4 edge_color;
    float edge_scale;

    float4 tex_tint;
    float4 spa_tint;
    float4 toon_tint;
};

struct pmx_flip_morph
{
    std::int32_t index;
    float weight;
};

struct pmx_impulse_morph
{
    std::int32_t index;
    std::uint8_t local_flag; // 0: OFF, 1: ON
    float3 translate_velocity;
    float3 rotate_torque;
};

struct pmx_morph
{
    std::string name_jp;
    std::string name_en;

    std::uint8_t control_panel;
    pmx_morph_type type;

    std::vector<pmx_group_morph> group_morphs;
    std::vector<pmx_vertex_morph> vertex_morphs;
    std::vector<pmx_bone_morph> bone_morphs;
    std::vector<pmx_uv_morph> uv_morphs;
    std::vector<pmx_material_morph> material_morphs;
    std::vector<pmx_flip_morph> flip_morphs;
    std::vector<pmx_impulse_morph> impulse_morphs;
};

enum pmx_frame_type : std::uint8_t
{
    PMX_FRAME_TYPE_BONE,
    PMX_FRAME_TYPE_MORPH
};

struct pmx_display_frame
{
    pmx_frame_type type;
    std::int32_t index;
};

struct pmx_display_data
{
    std::string name_jp;
    std::string name_en;

    std::uint8_t flag; // 0: normal frame, 1: special frame

    std::vector<pmx_display_frame> frames;
};

enum pmx_rigidbody_shape_type : std::uint8_t
{
    PMX_RIGIDBODY_SHAPE_TYPE_SPHERE,
    PMX_RIGIDBODY_SHAPE_TYPE_BOX,
    PMX_RIGIDBODY_SHAPE_TYPE_CAPSULE
};

enum pmx_rigidbody_mode : std::uint8_t
{
    PMX_RIGIDBODY_MODE_STATIC,
    PMX_RIGIDBODY_MODE_DYNAMIC,
    PMX_RIGIDBODY_MODE_MERGE
};

struct pmx_rigidbody
{
    std::string name_jp;
    std::string name_en;

    std::int32_t bone_index;
    std::uint8_t group;
    std::uint16_t collision_group;

    pmx_rigidbody_shape_type shape;
    float3 size;
    float3 translate;
    float4 rotate;
    float mass;
    float linear_damping;
    float angular_damping;
    float repulsion;
    float friction;

    pmx_rigidbody_mode mode;
};

enum pmx_joint_type : std::uint8_t
{
    PMX_JOINT_TYPE_SPRING_DOF6,
    PMX_JOINT_TYPE_DOF6,
    PMX_JOINT_TYPE_P2P,
    PMX_JOINT_TYPE_CONETWIST,
    PMX_JOINT_TYPE_SLIDER,
    PMX_JOINT_TYPE_HINGE
};

struct pmx_joint
{
    std::string name_jp;
    std::string name_en;

    pmx_joint_type type;
    std::int32_t rigidbody_a_index;
    std::int32_t rigidbody_b_index;

    float3 translate;
    float4 rotate;

    float3 translate_min;
    float3 translate_max;
    float3 rotate_min;
    float3 rotate_max;

    float3 spring_translate_factor;
    float3 spring_rotate_factor;
};

enum pmx_vertex_attribute
{
    PMX_VERTEX_ATTRIBUTE_POSITION,
    PMX_VERTEX_ATTRIBUTE_NORMAL,
    PMX_VERTEX_ATTRIBUTE_UV,
    PMX_VERTEX_ATTRIBUTE_EDGE,
    PMX_VERTEX_ATTRIBUTE_SKIN,
    PMX_VERTEX_ATTRIBUTE_BDEF_BONE,
    PMX_VERTEX_ATTRIBUTE_SDEF_BONE,
    PMX_VERTEX_ATTRIBUTE_NUM_TYPES
};

class pmx
{
public:
    pmx(std::string_view path);

    bool is_load() const noexcept { return m_loaded; }

public:
    struct submesh
    {
        std::size_t index_start;
        std::size_t index_count;

        std::size_t material_index;
    };

    struct bdef_data
    {
        uint4 index;
        float4 weight;
    };

    struct sdef_data
    {
        uint2 index;
        float2 weight;
        float3 center;
        float _padding_0;
        float3 r0;
        float _padding_1;
        float3 r1;
        float _padding_2;
    };

    pmx_header header;

    std::vector<float3> position;
    std::vector<float3> normal;
    std::vector<float2> uv;

    std::vector<uint2> skin; // first: skin type(0: BDEF, 1: SDEF), second: skin data index
    std::vector<bdef_data> bdef;
    std::vector<sdef_data> sdef;

    std::vector<std::int32_t> indices;

    std::vector<std::string> textures;
    std::vector<pmx_material> materials;

    std::vector<submesh> submeshes;

    std::vector<pmx_bone> bones;
    std::vector<pmx_morph> morphs;
    std::vector<pmx_display_data> display;

    std::vector<pmx_rigidbody> rigidbodies;
    std::vector<pmx_joint> joints;

private:
    bool load_header(std::ifstream& fin);
    bool load_mesh(std::ifstream& fin);
    bool load_material(std::ifstream& fin, std::string_view root_path);
    bool load_bone(std::ifstream& fin);
    bool load_morph(std::ifstream& fin);
    bool load_display(std::ifstream& fin);
    bool load_physics(std::ifstream& fin);

    std::int32_t read_index(std::ifstream& fin, std::uint8_t size);
    std::string read_text(std::ifstream& fin);

    bool m_loaded;
};
} // namespace violet::sample