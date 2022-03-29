#pragma once

#include "math.hpp"
#include <fstream>
#include <string>
#include <vector>

namespace ash::sample::mmd
{
enum class pmx_vertex_weight : std::uint8_t
{
    BDEF1,
    BDEF2,
    BDEF4,
    SDEF,
    QDEF,
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

struct pmx_vertex
{
    math::float3 position;
    math::float3 normal;
    math::float2 uv;
    math::uint4 bone;
    math::float3 weight;

    float edge_scale;
    std::vector<math::float4> add_uv;
};

enum class draw_flag : std::uint8_t
{
    NO_CULL = 0x01,
    GROUND_SHADOW = 0x02,
    DRAW_SHADOW = 0x04,
    RECEIVE_SHADOW = 0x08,
    HAS_EDGE = 0x10,
    VERTEX_COLOUR = 0x20,
    POINT_DRAWING = 0x40,
    LINE_DRAWING = 0x80,
};

enum class sphere_mode : std::uint8_t
{
    DISABLED = 0,
    MULTIPLY = 1,
    ADDITIVE = 2,
    ADDITIONAL_VEC4 = 3
};

enum class toon_mode : std::uint8_t
{
    TEXTURE,
    INTERNAL
};

struct pmx_material
{
    std::string name_jp;
    std::string name_en;

    math::float4 diffuse;
    math::float3 specular;
    float specular_strength;
    math::float3 ambient;
    draw_flag flag;
    math::float4 edge;
    float edge_scale;

    std::int32_t texture_index;
    sphere_mode sphere_mode;
    std::int32_t sphere_index;
    toon_mode toon_mode;
    std::int32_t toon_index;

    std::string meta_data;
    std::int32_t num_indices;
};

enum pmx_bone_flag : std::uint16_t
{
    INDEXED_TAIL_POSITION = 0x0001,
    ROTATABLE = 0x0002,
    TRANSLATABLE = 0x0004,
    VISIBLE = 0x0008,
    ENABLED = 0x0010,
    IK = 0x0020,
    INHERIT_LOCAL = 0x0080,
    INHERIT_ROTATION = 0x0100,
    INHERIT_TRANSLATION = 0x0200,
    FIXED_AXIS = 0x0400,
    LOCAL_AXIS = 0x0800,
    PHYSICS_AFTER_DEFORM = 0x1000,
    EXTERNAL_PARENT_DEFORM = 0x2000
};

struct pmx_ik_link
{
    std::int32_t bone_index;
    bool enable_limit;
    math::float3 limit_min;
    math::float3 limit_max;
};

struct pmx_bone
{
    std::string name_jp;
    std::string name_en;

    math::float3 position;
    std::int32_t parent_index;
    std::int32_t layer;
    pmx_bone_flag flags;
    math::float3 tail_position;
    std::int32_t tail_index;
    std::int32_t inherit_index;
    float inherit_weight;
    math::float3 fixed_axis;
    math::float3 local_x_axis;
    math::float3 local_z_axis;
    std::int32_t external_parent_index;

    std::int32_t ik_target_index;
    std::int32_t ik_iteration_count;
    float ik_limit;
    std::vector<pmx_ik_link> ik_links;
};

enum class pmx_morph_type : std::uint8_t
{
    GROUP,
    VERTEX,
    BONE,
    UV,
    EXT_UV1,
    EXT_UV2,
    EXT_UV3,
    EXT_UV4,
    MATERIAL,
    FLIP,
    IMPULSE
};

struct pmx_group_morph
{
    std::int32_t index;
    float weight;
};

struct pmx_vertex_morph
{
    std::int32_t index;
    math::float3 translation;
};

struct pmx_bone_morph
{
    std::int32_t index;
    math::float3 translation;
    math::float4 rotation;
};

struct pmx_uv_morph
{
    std::int32_t index;
    math::float4 value;
};

struct pmx_material_morph
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

struct pmx_flip_morph
{
    std::int32_t index;
    float weight;
};

struct pmx_impulse_morph
{
    std::int32_t index;
    std::uint8_t local_flag; // 0: OFF, 1: ON
    math::float3 translate_velocity;
    math::float3 rotate_torque;
};

struct pmx_morph
{
    std::string name_jp;
    std::string name_en;

    std::uint8_t control_panel;
    pmx_morph_type type;

    std::vector<pmx_group_morph> group_morph;
    std::vector<pmx_vertex_morph> vertex_morph;
    std::vector<pmx_bone_morph> bone_morph;
    std::vector<pmx_uv_morph> uv_morph;
    std::vector<pmx_material_morph> material_morph;
    std::vector<pmx_flip_morph> flip_morph;
    std::vector<pmx_impulse_morph> impulse_morph;
};

enum class pmx_frame_type : std::uint8_t
{
    BONE,
    MORPH
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

enum class pmx_rigidbody_shape_type : std::uint8_t
{
    SPHERE,
    BOX,
    CAPSULE
};

enum class pmx_rigidbody_mode : std::uint8_t
{
    STATIC,
    DYNAMIC,
    MERGE
};

struct pmx_rigidbody
{
    std::string name_jp;
    std::string name_en;

    std::int32_t bone_index;
    std::uint8_t group;
    std::uint16_t collision_group;

    pmx_rigidbody_shape_type shape;
    math::float3 size;
    math::float3 translate;
    math::float3 rotate;
    float mass;
    float translateDimmer;
    float rotateDimmer;
    float repulsion;
    float friction;

    pmx_rigidbody_mode mode;
};

enum class pmx_joint_type : std::uint8_t
{
    SPRING_DOF6,
    DOF6,
    P2P,
    CONETWIST,
    SLIDER,
    HINGE
};

struct pmx_joint
{
    std::string name_jp;
    std::string name_en;

    pmx_joint_type type;
    std::int32_t rigidbody_a_index;
    std::int32_t rigidbody_b_index;

    math::float3 translate;
    math::float3 rotate;

    math::float3 translate_min;
    math::float3 translate_max;
    math::float3 rotate_min;
    math::float3 rotate_max;

    math::float3 spring_translate_factor;
    math::float3 spring_rotate_factor;
};

class pmx_loader
{
public:
    pmx_loader();
    bool load(std::string_view path);

    const std::vector<pmx_vertex>& vertices() const noexcept { return m_vertices; }
    const std::vector<std::int32_t>& indices() const noexcept { return m_indices; }

    std::vector<std::pair<std::size_t, std::size_t>> submesh() const noexcept;

    const std::vector<pmx_material>& materials() const { return m_materials; }
    const std::vector<std::string>& textures() const { return m_textures; }

    const std::vector<std::string>& internal_toon() const { return m_internal_toon; }

private:
    bool load_header(std::ifstream& fin);
    bool load_vertex(std::ifstream& fin);
    bool load_index(std::ifstream& fin);
    bool load_texture(std::ifstream& fin);
    bool load_material(std::ifstream& fin);
    bool load_bone(std::ifstream& fin);
    bool load_morph(std::ifstream& fin);
    bool load_display_frame(std::ifstream& fin);
    bool load_rigidbody(std::ifstream& fin);
    bool load_joint(std::ifstream& fin);

    std::int32_t read_index(std::ifstream& fin, std::uint8_t size);
    std::string read_text(std::ifstream& fin);

    pmx_header m_header;

    std::vector<pmx_vertex> m_vertices;
    std::vector<std::int32_t> m_indices;
    std::vector<std::string> m_textures;

    std::vector<pmx_material> m_materials;
    std::vector<pmx_bone> m_bones;
    std::vector<pmx_morph> m_morphs;
    std::vector<pmx_display_data> m_display_frames;
    std::vector<pmx_rigidbody> m_rigidbodys;
    std::vector<pmx_joint> m_joints;

    std::vector<std::string> m_internal_toon;
};
} // namespace ash::sample::mmd