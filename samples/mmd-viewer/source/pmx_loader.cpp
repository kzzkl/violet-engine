#include "pmx_loader.hpp"
#include "encode.hpp"
#include "graphics/rhi.hpp"
#include "log.hpp"
#include "physics/physics.hpp"
#include <fstream>
#include <map>

namespace ash::sample::mmd
{
template <typename T>
static void read(std::istream& fin, T& dest)
{
    fin.read(reinterpret_cast<char*>(&dest), sizeof(T));
}

pmx_loader::pmx_loader() noexcept
{
}

bool pmx_loader::load(
    std::string_view path,
    const std::vector<graphics::resource_interface*>& internal_toon)
{
    std::ifstream fin(path.data(), std::ios::binary);
    if (!fin.is_open())
        return false;

    std::string_view root_path = path.substr(0, path.find_last_of('/'));

    if (!load_header(fin))
        return false;

    if (!load_vertex(fin))
        return false;

    if (!load_index(fin))
        return false;

    if (!load_texture(fin, root_path))
        return false;

    if (!load_material(fin, internal_toon))
        return false;

    if (!load_bone(fin))
        return false;

    if (!load_morph(fin))
        return false;

    if (!load_display_frame(fin))
        return false;

    if (!load_rigidbody(fin))
        return false;

    if (!load_joint(fin))
        return false;

    return true;
}

bool pmx_loader::load_header(std::ifstream& fin)
{
    char magic[4];
    fin.read(magic, 4);

    read<float>(fin, m_header.version);

    std::uint8_t ignore;
    read<std::uint8_t>(fin, ignore); // ignore global size
    read<std::uint8_t>(fin, m_header.text_encoding);
    read<std::uint8_t>(fin, m_header.num_add_vec4);
    read<std::uint8_t>(fin, m_header.vertex_index_size);
    read<std::uint8_t>(fin, m_header.texture_index_size);
    read<std::uint8_t>(fin, m_header.material_index_size);
    read<std::uint8_t>(fin, m_header.bone_index_size);
    read<std::uint8_t>(fin, m_header.morph_index_size);
    read<std::uint8_t>(fin, m_header.rigidbody_index_size);

    m_header.name_jp = read_text(fin);
    m_header.name_en = read_text(fin);
    m_header.comments_jp = read_text(fin);
    m_header.comments_en = read_text(fin);

    return true;
}

bool pmx_loader::load_vertex(std::ifstream& fin)
{
    std::int32_t vertex_count;
    read<std::int32_t>(fin, vertex_count);
    m_vertex_count = vertex_count;

    std::vector<pmx_vertex> vertices(m_vertex_count);

    for (auto& v : vertices)
        v.add_uv.resize(m_header.num_add_vec4);

    for (pmx_vertex& vertex : vertices)
    {
        read<math::float3>(fin, vertex.position);
        read<math::float3>(fin, vertex.normal);
        read<math::float2>(fin, vertex.uv);

        for (std::uint8_t i = 0; i < m_header.num_add_vec4; ++i)
        {
            read<math::float4>(fin, vertex.add_uv[i]);
        }

        pmx_vertex_weight weight_type;
        read<pmx_vertex_weight>(fin, weight_type);

        float unused;
        switch (weight_type)
        {
        case pmx_vertex_weight::BDEF1: {
            vertex.bone.data[0] = read_index(fin, m_header.bone_index_size);
            vertex.weight.data[0] = 1.0f;
            break;
        }
        case pmx_vertex_weight::BDEF2: {
            vertex.bone.data[0] = read_index(fin, m_header.bone_index_size);
            vertex.bone.data[1] = read_index(fin, m_header.bone_index_size);
            read<float>(fin, vertex.weight.data[0]);
            vertex.weight.data[1] = 1.0f - vertex.weight.data[0];
            break;
        }
        case pmx_vertex_weight::BDEF4: {
            vertex.bone.data[0] = read_index(fin, m_header.bone_index_size);
            vertex.bone.data[1] = read_index(fin, m_header.bone_index_size);
            vertex.bone.data[2] = read_index(fin, m_header.bone_index_size);
            vertex.bone.data[3] = read_index(fin, m_header.bone_index_size);
            read<float>(fin, vertex.weight.data[0]);
            read<float>(fin, vertex.weight.data[1]);
            read<float>(fin, vertex.weight.data[2]);
            read<float>(fin, unused);
            break;
        }
        case pmx_vertex_weight::SDEF: {
            vertex.bone.data[0] = read_index(fin, m_header.bone_index_size);
            vertex.bone.data[1] = read_index(fin, m_header.bone_index_size);
            read<float>(fin, vertex.weight.data[0]);
            vertex.weight.data[1] = 1.0f - vertex.weight.data[0];

            math::float3 ignore;
            read<math::float3>(fin, ignore);
            read<math::float3>(fin, ignore);
            read<math::float3>(fin, ignore);
            break;
        }
        case pmx_vertex_weight::QDEF: {
            vertex.bone.data[0] = read_index(fin, m_header.bone_index_size);
            vertex.bone.data[1] = read_index(fin, m_header.bone_index_size);
            vertex.bone.data[2] = read_index(fin, m_header.bone_index_size);
            vertex.bone.data[3] = read_index(fin, m_header.bone_index_size);
            read<float>(fin, vertex.weight.data[0]);
            read<float>(fin, vertex.weight.data[1]);
            read<float>(fin, vertex.weight.data[2]);
            read<float>(fin, unused);
            break;
        }
        default:
            break;
        }

        read<float>(fin, vertex.edge_scale);
    }

    // Make vertex buffer.
    std::vector<math::float3> position;
    std::vector<math::float3> normal;
    std::vector<math::float2> uv;
    std::vector<math::uint4> bone;
    std::vector<math::float3> bone_weight;

    position.reserve(m_vertex_count);
    normal.reserve(m_vertex_count);
    uv.reserve(m_vertex_count);
    bone.reserve(m_vertex_count);
    bone_weight.reserve(m_vertex_count);
    for (const pmx_vertex& v : vertices)
    {
        position.push_back(v.position);
        normal.push_back(v.normal);
        uv.push_back(v.uv);
        bone.push_back(v.bone);
        bone_weight.push_back(v.weight);
    }

    m_vertex_buffers.resize(PMX_VERTEX_ATTRIBUTE_NUM_TYPES);

    m_vertex_buffers[PMX_VERTEX_ATTRIBUTE_POSITION] = graphics::rhi::make_vertex_buffer(
        position.data(),
        position.size(),
        graphics::VERTEX_BUFFER_FLAG_COMPUTE_IN);
    m_vertex_buffers[PMX_VERTEX_ATTRIBUTE_NORMAL] = graphics::rhi::make_vertex_buffer(
        normal.data(),
        normal.size(),
        graphics::VERTEX_BUFFER_FLAG_COMPUTE_IN);
    m_vertex_buffers[PMX_VERTEX_ATTRIBUTE_UV] = graphics::rhi::make_vertex_buffer(
        uv.data(),
        uv.size(),
        graphics::VERTEX_BUFFER_FLAG_COMPUTE_IN);
    m_vertex_buffers[PMX_VERTEX_ATTRIBUTE_BONE] = graphics::rhi::make_vertex_buffer(
        bone.data(),
        bone.size(),
        graphics::VERTEX_BUFFER_FLAG_COMPUTE_IN);
    m_vertex_buffers[PMX_VERTEX_ATTRIBUTE_BONE_WEIGHT] = graphics::rhi::make_vertex_buffer(
        bone_weight.data(),
        bone_weight.size(),
        graphics::VERTEX_BUFFER_FLAG_COMPUTE_IN);

    return true;
}

bool pmx_loader::load_index(std::ifstream& fin)
{
    std::int32_t index_count;
    read<std::int32_t>(fin, index_count);
    std::vector<std::int32_t> indices;
    indices.reserve(index_count);

    for (std::int32_t i = 0; i < index_count; ++i)
        indices.push_back(read_index(fin, m_header.vertex_index_size));

    // Make index buffer.
    m_index_buffer = graphics::rhi::make_index_buffer(indices.data(), indices.size());

    return true;
}

bool pmx_loader::load_texture(std::ifstream& fin, std::string_view root_path)
{
    std::int32_t texture_count;
    read<std::int32_t>(fin, texture_count);

    for (std::int32_t i = 0; i < texture_count; ++i)
    {
        std::string path = std::string(root_path) + "/" + read_text(fin);
        m_textures.push_back(graphics::rhi::make_texture(path));
    }

    return true;
}

bool pmx_loader::load_material(
    std::ifstream& fin,
    const std::vector<graphics::resource_interface*>& internal_toon)
{
    std::int32_t material_count;
    read<std::int32_t>(fin, material_count);

    std::vector<pmx_material> materials(material_count);
    for (auto& mat : materials)
    {
        mat.name_jp = read_text(fin);
        mat.name_en = read_text(fin);

        read<math::float4>(fin, mat.diffuse);
        read<math::float3>(fin, mat.specular);
        read<float>(fin, mat.specular_strength);
        read<math::float3>(fin, mat.ambient);
        read<draw_flag>(fin, mat.flag);
        read<math::float4>(fin, mat.edge_color);
        read<float>(fin, mat.edge_size);
        mat.texture_index = read_index(fin, m_header.texture_index_size);
        mat.sphere_index = read_index(fin, m_header.texture_index_size);

        read<sphere_mode>(fin, mat.sphere_mode);
        read<toon_mode>(fin, mat.toon_mode);

        if (mat.toon_mode == toon_mode::TEXTURE)
        {
            mat.toon_index = read_index(fin, m_header.texture_index_size);
        }
        else if (mat.toon_mode == toon_mode::INTERNAL)
        {
            std::uint8_t toon_index;
            read<std::uint8_t>(fin, toon_index);
            mat.toon_index = toon_index;
        }
        else
        {
            return false;
        }

        mat.meta_data = read_text(fin);
        read<std::int32_t>(fin, mat.index_count);
    }

    m_submesh.reserve(materials.size());
    std::size_t offset = 0;
    for (auto& material : materials)
    {
        m_submesh.push_back(std::make_pair(offset, offset + material.index_count));
        offset += material.index_count;
    }

    for (auto& mmd_material : materials)
    {
        auto parameter = std::make_unique<material_pipeline_parameter>();
        parameter->diffuse(mmd_material.diffuse);
        parameter->specular(mmd_material.specular);
        parameter->specular_strength(mmd_material.specular_strength);
        parameter->edge_color(mmd_material.edge_color);
        parameter->edge_size(mmd_material.edge_size);
        parameter->toon_mode(mmd_material.toon_index == -1 ? std::uint32_t(0) : std::uint32_t(1));
        parameter->spa_mode(static_cast<std::uint32_t>(mmd_material.sphere_mode));

        if (mmd_material.texture_index != -1)
            parameter->tex(m_textures[mmd_material.texture_index].get());

        if (mmd_material.toon_index != -1)
        {
            if (mmd_material.toon_mode == toon_mode::TEXTURE)
                parameter->toon(m_textures[mmd_material.toon_index].get());
            else if (mmd_material.toon_mode == toon_mode::INTERNAL)
                parameter->toon(internal_toon[mmd_material.toon_index]);
            else
                throw std::runtime_error("Invalid toon mode");
        }
        if (mmd_material.sphere_mode != sphere_mode::DISABLED)
            parameter->spa(m_textures[mmd_material.sphere_index].get());

        m_materials.push_back(std::move(parameter));
    }

    return true;
}

bool pmx_loader::load_bone(std::ifstream& fin)
{
    std::int32_t bone_count;
    read<std::int32_t>(fin, bone_count);

    m_bones.resize(bone_count);
    for (pmx_bone& bone : m_bones)
    {
        bone.name_jp = read_text(fin);
        bone.name_en = read_text(fin);

        read<math::float3>(fin, bone.position);
        bone.parent_index = read_index(fin, m_header.bone_index_size);
        read<std::int32_t>(fin, bone.layer);

        read<pmx_bone_flag>(fin, bone.flags);

        if (bone.flags & pmx_bone_flag::INDEXED_TAIL_POSITION)
            bone.tail_index = read_index(fin, m_header.bone_index_size);
        else
            read<math::float3>(fin, bone.tail_position);

        if (bone.flags & pmx_bone_flag::INHERIT_ROTATION ||
            bone.flags & pmx_bone_flag::INHERIT_TRANSLATION)
        {
            bone.inherit_index = read_index(fin, m_header.bone_index_size);
            read<float>(fin, bone.inherit_weight);
        }

        if (bone.flags & pmx_bone_flag::FIXED_AXIS)
            read<math::float3>(fin, bone.fixed_axis);

        if (bone.flags & pmx_bone_flag::LOCAL_AXIS)
        {
            read<math::float3>(fin, bone.local_x_axis);
            read<math::float3>(fin, bone.local_z_axis);
        }

        if (bone.flags & pmx_bone_flag::EXTERNAL_PARENT_DEFORM)
            bone.external_parent_index = read_index(fin, m_header.bone_index_size);

        if (bone.flags & pmx_bone_flag::IK)
        {
            bone.ik_target_index = read_index(fin, m_header.bone_index_size);
            read<std::int32_t>(fin, bone.ik_loop_count);
            read<float>(fin, bone.ik_limit);

            std::int32_t count;
            read<std::int32_t>(fin, count);
            bone.ik_links.resize(count);

            for (pmx_ik_link& link : bone.ik_links)
            {
                link.bone_index = read_index(fin, m_header.bone_index_size);

                unsigned char hasLimit;
                read<unsigned char>(fin, hasLimit);
                link.enable_limit = hasLimit != 0;

                if (link.enable_limit)
                {
                    read<math::float3>(fin, link.limit_min);
                    read<math::float3>(fin, link.limit_max);
                }
            }
        }
    }

    return true;
}

bool pmx_loader::load_morph(std::ifstream& fin)
{
    std::int32_t morph_count;
    read<std::int32_t>(fin, morph_count);

    m_morphs.resize(morph_count);

    for (pmx_morph& morph : m_morphs)
    {
        morph.name_jp = read_text(fin);
        morph.name_en = read_text(fin);

        read<std::uint8_t>(fin, morph.control_panel);
        read<pmx_morph_type>(fin, morph.type);

        std::int32_t count;
        read<std::int32_t>(fin, count);

        switch (morph.type)
        {
        case pmx_morph_type::GROUP: {
            morph.group_morphs.resize(count);
            for (auto& group : morph.group_morphs)
            {
                group.index = read_index(fin, m_header.morph_index_size);
                read<float>(fin, group.weight);
            }
            break;
        }
        case pmx_morph_type::VERTEX: {
            morph.vertex_morphs.resize(count);
            for (auto& vertex_morph : morph.vertex_morphs)
            {
                vertex_morph.index = read_index(fin, m_header.vertex_index_size);
                read<math::float3>(fin, vertex_morph.translation);
            }
            break;
        }
        case pmx_morph_type::BONE: {
            morph.bone_morphs.resize(count);
            for (auto& bone_morph : morph.bone_morphs)
            {
                bone_morph.index = read_index(fin, m_header.bone_index_size);
                read<math::float3>(fin, bone_morph.translation);
                read<math::float4>(fin, bone_morph.rotation);
            }
            break;
        }
        case pmx_morph_type::UV:
        case pmx_morph_type::UV_EXT_1:
        case pmx_morph_type::UV_EXT_2:
        case pmx_morph_type::UV_EXT_3:
        case pmx_morph_type::UV_EXT_4: {
            morph.uv_morphs.resize(count);
            for (auto& uv_morph : morph.uv_morphs)
            {
                uv_morph.index = read_index(fin, m_header.vertex_index_size);
                read<math::float4>(fin, uv_morph.uv);
            }
            break;
        }
        case pmx_morph_type::MATERIAL: {
            morph.material_morphs.resize(count);
            for (auto& material_morph : morph.material_morphs)
            {
                material_morph.index = read_index(fin, m_header.material_index_size);
                read<std::uint8_t>(fin, material_morph.operate);
                read<math::float4>(fin, material_morph.diffuse);
                read<math::float3>(fin, material_morph.specular);
                read<float>(fin, material_morph.specular_strength);
                read<math::float3>(fin, material_morph.ambient);
                read<math::float4>(fin, material_morph.edge_color);
                read<float>(fin, material_morph.edge_scale);
                read<math::float4>(fin, material_morph.tex_tint);
                read<math::float4>(fin, material_morph.spa_tint);
                read<math::float4>(fin, material_morph.toon_tint);
            }
            break;
        }
        case pmx_morph_type::FLIP: {
            morph.flip_morphs.resize(count);
            for (auto& flip_morph : morph.flip_morphs)
            {
                flip_morph.index = read_index(fin, m_header.morph_index_size);
                read<float>(fin, flip_morph.weight);
            }
            break;
        }
        case pmx_morph_type::IMPULSE: {
            morph.impulse_morphs.resize(count);
            for (auto& impulse_morph : morph.impulse_morphs)
            {
                impulse_morph.index = read_index(fin, m_header.rigidbody_index_size);
                read<std::uint8_t>(fin, impulse_morph.local_flag);
                read<math::float3>(fin, impulse_morph.translate_velocity);
                read<math::float3>(fin, impulse_morph.rotate_torque);
            }
            break;
        }
        default:
            return false;
        }
    }

    return true;
}

bool pmx_loader::load_display_frame(std::ifstream& fin)
{
    std::int32_t numDisplayFrame;
    read<std::int32_t>(fin, numDisplayFrame);
    m_display_frames.resize(numDisplayFrame);

    for (pmx_display_data& display_frame : m_display_frames)
    {
        display_frame.name_jp = read_text(fin);
        display_frame.name_en = read_text(fin);

        read<std::uint8_t>(fin, display_frame.flag);

        std::int32_t numFrames;
        read<std::int32_t>(fin, numFrames);
        display_frame.frames.resize(numFrames);

        for (pmx_display_frame& frame : display_frame.frames)
        {
            read<pmx_frame_type>(fin, frame.type);
            switch (frame.type)
            {
            case pmx_frame_type::BONE:
                frame.index = read_index(fin, m_header.bone_index_size);
                break;
            case pmx_frame_type::MORPH:
                frame.index = read_index(fin, m_header.morph_index_size);
                break;
            default:
                return false;
            }
        }
    }

    return true;
}

bool pmx_loader::load_rigidbody(std::ifstream& fin)
{
    std::int32_t rigidbody_count;
    read<std::int32_t>(fin, rigidbody_count);
    m_rigidbodies.resize(rigidbody_count);

    for (pmx_rigidbody& rigidbody : m_rigidbodies)
    {
        rigidbody.name_jp = read_text(fin);
        rigidbody.name_en = read_text(fin);

        rigidbody.bone_index = read_index(fin, m_header.bone_index_size);
        read<std::uint8_t>(fin, rigidbody.group);
        read<std::uint16_t>(fin, rigidbody.collision_group);

        read<pmx_rigidbody_shape_type>(fin, rigidbody.shape);
        read<math::float3>(fin, rigidbody.size);

        read<math::float3>(fin, rigidbody.translate);

        math::float3 rotate;
        read<math::float3>(fin, rotate);
        rigidbody.rotate = math::quaternion::rotation_euler(rotate[1], rotate[0], rotate[2]);

        read<float>(fin, rigidbody.mass);
        read<float>(fin, rigidbody.translate_dimmer);
        read<float>(fin, rigidbody.rotate_dimmer);
        read<float>(fin, rigidbody.repulsion);
        read<float>(fin, rigidbody.friction);
        read<pmx_rigidbody_mode>(fin, rigidbody.mode);
    }

    auto& physics = system<physics::physics>();
    auto make_shape = [&](const pmx_rigidbody& pmx_rigidbody)
        -> std::unique_ptr<physics::collision_shape_interface> {
        physics::collision_shape_desc desc = {};
        switch (pmx_rigidbody.shape)
        {
        case pmx_rigidbody_shape_type::SPHERE:
            desc.type = physics::collision_shape_type::SPHERE;
            desc.sphere.radius = pmx_rigidbody.size[0];
            break;
        case pmx_rigidbody_shape_type::BOX:
            desc.type = physics::collision_shape_type::BOX;
            desc.box.length = pmx_rigidbody.size[0] * 2.0f;
            desc.box.height = pmx_rigidbody.size[1] * 2.0f;
            desc.box.width = pmx_rigidbody.size[2] * 2.0f;
            break;
        case pmx_rigidbody_shape_type::CAPSULE:
            desc.type = physics::collision_shape_type::CAPSULE;
            desc.capsule.radius = pmx_rigidbody.size[0];
            desc.capsule.height = pmx_rigidbody.size[1];
            break;
        default:
            return nullptr;
        };
        return physics.make_shape(desc);
    };

    for (pmx_rigidbody& rigidbody : m_rigidbodies)
    {
        m_collision_shapes.push_back(make_shape(rigidbody));
    }

    return true;
}

bool pmx_loader::load_joint(std::ifstream& fin)
{
    std::int32_t numJoint;
    read<std::int32_t>(fin, numJoint);
    m_joints.resize(numJoint);

    for (pmx_joint& joint : m_joints)
    {
        joint.name_jp = read_text(fin);
        joint.name_en = read_text(fin);

        read<pmx_joint_type>(fin, joint.type);
        joint.rigidbody_a_index = read_index(fin, m_header.rigidbody_index_size);
        joint.rigidbody_b_index = read_index(fin, m_header.rigidbody_index_size);

        read<math::float3>(fin, joint.translate);
        math::float3 rotate;
        read<math::float3>(fin, rotate);
        joint.rotate = math::quaternion::rotation_euler(rotate[1], rotate[0], rotate[2]);

        read<math::float3>(fin, joint.translate_min);
        read<math::float3>(fin, joint.translate_max);
        read<math::float3>(fin, joint.rotate_min);
        read<math::float3>(fin, joint.rotate_max);

        read<math::float3>(fin, joint.spring_translate_factor);
        read<math::float3>(fin, joint.spring_rotate_factor);
    }

    return true;
}

std::int32_t pmx_loader::read_index(std::ifstream& fin, std::uint8_t size)
{
    switch (size)
    {
    case 1: {
        std::uint8_t res;
        read<std::uint8_t>(fin, res);
        if (res != 0xFF)
            return static_cast<std::int32_t>(res);
        else
            return -1;
    }
    case 2: {
        std::uint16_t res;
        read<std::uint16_t>(fin, res);
        if (res != 0xFFFF)
            return static_cast<std::int32_t>(res);
        else
            return -1;
    }
    case 4: {
        std::int32_t res;
        read<std::int32_t>(fin, res);
        if (res != 0xFFFFFFFF)
            return static_cast<std::int32_t>(res);
        else
            return -1;
    }
    default:
        return 0;
    }
}

std::string pmx_loader::read_text(std::ifstream& fin)
{
    std::int32_t len;
    read<std::int32_t>(fin, len);
    if (len > 0)
    {
        std::string result;
        if (m_header.text_encoding == 1)
        {
            result.resize(len);
            fin.read(result.data(), len);
        }
        else if (m_header.text_encoding == 0)
        {
            std::u16string str;
            str.resize(len);
            fin.read(reinterpret_cast<char*>(str.data()), len);
            convert<encode_type::UTF16, encode_type::UTF8>(str, result);
        }
        return result;
    }
    else
    {
        return "";
    }
}
} // namespace ash::sample::mmd