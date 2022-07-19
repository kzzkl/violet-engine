#pragma once

#include "math/math.hpp"
#include "mmd_pipeline.hpp"
#include "physics_interface.hpp"
#include <fstream>
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
    math::float4 edge_color;
    float edge_size;

    std::int32_t texture_index;
    sphere_mode sphere_mode;
    std::int32_t sphere_index;
    toon_mode toon_mode;
    std::int32_t toon_index;

    std::string meta_data;
    std::int32_t index_count;
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
    std::int32_t ik_loop_count;
    float ik_limit;
    std::vector<pmx_ik_link> ik_links;
};

enum class pmx_morph_type : std::uint8_t
{
    GROUP,
    VERTEX,
    BONE,
    UV,
    UV_EXT_1,
    UV_EXT_2,
    UV_EXT_3,
    UV_EXT_4,
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
    math::float4 uv;
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

    std::vector<pmx_group_morph> group_morphs;
    std::vector<pmx_vertex_morph> vertex_morphs;
    std::vector<pmx_bone_morph> bone_morphs;
    std::vector<pmx_uv_morph> uv_morphs;
    std::vector<pmx_material_morph> material_morphs;
    std::vector<pmx_flip_morph> flip_morphs;
    std::vector<pmx_impulse_morph> impulse_morphs;
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
    math::float4 rotate;
    float mass;
    float translate_dimmer;
    float rotate_dimmer;
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
    math::float4 rotate;

    math::float3 translate_min;
    math::float3 translate_max;
    math::float3 rotate_min;
    math::float3 rotate_max;

    math::float3 spring_translate_factor;
    math::float3 spring_rotate_factor;
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

class pmx_loader
{
public:
    pmx_loader() noexcept;
    bool load(
        std::string_view path,
        const std::vector<graphics::resource_interface*>& internal_toon);

    std::size_t vertex_count() const noexcept { return m_vertex_count; }
    graphics::resource_interface* vertex_buffers(pmx_vertex_attribute attribute) const
    {
        return m_vertex_buffers[attribute].get();
    }
    graphics::resource_interface* index_buffer() const { return m_index_buffer.get(); }

    const std::vector<std::pair<std::size_t, std::size_t>>& submesh() const noexcept
    {
        return m_submesh;
    }

    material_pipeline_parameter* materials(std::size_t index) const noexcept
    {
        return m_materials[index].get();
    }
    graphics::resource_interface* texture(std::size_t index) const
    {
        return m_textures[index].get();
    }

    const std::vector<pmx_bone>& bones() const noexcept { return m_bones; }
    const std::vector<pmx_morph>& morph() const noexcept { return m_morphs; }

    physics::collision_shape_interface* collision_shape(std::size_t index) const noexcept
    {
        return m_collision_shapes[index].get();
    }
    const std::vector<pmx_rigidbody>& rigidbodies() const noexcept { return m_rigidbodies; }
    const std::vector<pmx_joint>& joints() const noexcept { return m_joints; }

private:
    bool load_header(std::ifstream& fin);
    bool load_vertex(std::ifstream& fin);
    bool load_index(std::ifstream& fin);
    bool load_texture(std::ifstream& fin, std::string_view root_path);
    bool load_material(
        std::ifstream& fin,
        const std::vector<graphics::resource_interface*>& internal_toon);
    bool load_bone(std::ifstream& fin);
    bool load_morph(std::ifstream& fin);
    bool load_display_frame(std::ifstream& fin);
    bool load_rigidbody(std::ifstream& fin);
    bool load_joint(std::ifstream& fin);

    std::int32_t read_index(std::ifstream& fin, std::uint8_t size);
    std::string read_text(std::ifstream& fin);

    pmx_header m_header;

    std::vector<pmx_bone> m_bones;
    std::vector<pmx_morph> m_morphs;
    std::vector<pmx_display_data> m_display_frames;
    std::vector<pmx_rigidbody> m_rigidbodies;
    std::vector<pmx_joint> m_joints;

    std::vector<std::unique_ptr<graphics::resource_interface>> m_vertex_buffers;
    std::size_t m_vertex_count;

    std::unique_ptr<graphics::resource_interface> m_index_buffer;
    std::vector<std::pair<std::size_t, std::size_t>> m_submesh;

    std::vector<std::unique_ptr<graphics::resource_interface>> m_textures;
    std::vector<std::unique_ptr<material_pipeline_parameter>> m_materials;

    std::vector<std::unique_ptr<physics::collision_shape_interface>> m_collision_shapes;
};
} // namespace ash::sample::mmd