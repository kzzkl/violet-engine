#include "pmx_loader.hpp"
#include "encode.hpp"
#include <fstream>

namespace violet::sample
{
template <typename T>
static void read(std::istream& fin, T& dest)
{
    fin.read(reinterpret_cast<char*>(&dest), sizeof(T));
}

pmx_loader::pmx_loader()
{
}

bool pmx_loader::load(std::string_view path)
{
    std::ifstream fin(path.data(), std::ios::binary);
    if (!fin.is_open())
        return false;

    if (!load_header(fin))
        return false;

    if (!load_mesh(fin))
        return false;

    if (!load_material(fin, path.substr(0, path.find_last_of('/'))))
        return false;

    if (!load_bone(fin))
        return false;

    if (!load_morph(fin))
        return false;

    if (!load_display(fin))
        return false;

    if (!load_physics(fin))
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

bool pmx_loader::load_mesh(std::ifstream& fin)
{
    std::int32_t vertex_count;
    read<std::int32_t>(fin, vertex_count);

    m_mesh.position.resize(vertex_count);
    m_mesh.normal.resize(vertex_count);
    m_mesh.uv.resize(vertex_count);

    // first: skin type(0: BDEF, 1: SDEF), second: skin data index
    std::vector<uint2> skin(vertex_count);
    std::vector<float> edge(vertex_count);

    std::vector<std::vector<float4>> add_uv(vertex_count);
    for (auto& v : add_uv)
        v.resize(m_header.num_add_vec4);

    // BDEF.
    struct bdef_data
    {
        uint4 index;
        float4 weight;
    };
    std::vector<bdef_data> bdef_bone;

    // SDEF.
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
    std::vector<sdef_data> sdef_bone;

    for (std::size_t i = 0; i < vertex_count; ++i)
    {
        read<float3>(fin, m_mesh.position[i]);
        read<float3>(fin, m_mesh.normal[i]);
        read<float2>(fin, m_mesh.uv[i]);

        for (std::uint8_t j = 0; j < m_header.num_add_vec4; ++j)
            read<float4>(fin, add_uv[i][j]);

        pmx_vertex_type weight_type;
        read<pmx_vertex_type>(fin, weight_type);

        switch (weight_type)
        {
        case PMX_VERTEX_TYPE_BDEF1: {
            bdef_data data = {};
            data.index[0] = read_index(fin, m_header.bone_index_size);
            data.weight = {1.0f, 0.0f, 0.0f, 0.0f};

            skin[i][0] = 0;
            skin[i][1] = bdef_bone.size();
            bdef_bone.push_back(data);
            break;
        }
        case PMX_VERTEX_TYPE_BDEF2: {
            bdef_data data = {};
            data.index[0] = read_index(fin, m_header.bone_index_size);
            data.index[1] = read_index(fin, m_header.bone_index_size);

            read<float>(fin, data.weight[0]);
            data.weight[1] = 1.0f - data.weight[0];

            skin[i][0] = 0;
            skin[i][1] = bdef_bone.size();
            bdef_bone.push_back(data);
            break;
        }
        case PMX_VERTEX_TYPE_BDEF4: {
            bdef_data data = {};
            data.index[0] = read_index(fin, m_header.bone_index_size);
            data.index[1] = read_index(fin, m_header.bone_index_size);
            data.index[2] = read_index(fin, m_header.bone_index_size);
            data.index[3] = read_index(fin, m_header.bone_index_size);
            read<float>(fin, data.weight[0]);
            read<float>(fin, data.weight[1]);
            read<float>(fin, data.weight[2]);
            read<float>(fin, data.weight[3]);

            skin[i][0] = 0;
            skin[i][1] = bdef_bone.size();
            bdef_bone.push_back(data);
            break;
        }
        case PMX_VERTEX_TYPE_SDEF: {
            sdef_data data = {};
            data.index[0] = read_index(fin, m_header.bone_index_size);
            data.index[1] = read_index(fin, m_header.bone_index_size);
            read<float>(fin, data.weight[0]);
            data.weight[1] = 1.0f - data.weight[0];
            read<float3>(fin, data.center);
            read<float3>(fin, data.r0);
            read<float3>(fin, data.r1);

            float4_simd center = simd::load(data.center);
            float4_simd r0 = simd::load(data.r0);
            float4_simd r1 = simd::load(data.r1);
            float4_simd rw = vector_simd::add(
                vector_simd::mul(r0, data.weight[0]),
                vector_simd::mul(r1, data.weight[1]));
            r0 = vector_simd::add(center, r0);
            r0 = vector_simd::sub(r0, rw);
            r0 = vector_simd::add(center, r0);
            r0 = vector_simd::mul(r0, 0.5f);
            r1 = vector_simd::add(center, r1);
            r1 = vector_simd::sub(r1, rw);
            r1 = vector_simd::add(center, r1);
            r1 = vector_simd::mul(r1, 0.5f);

            simd::store(r0, data.r0);
            simd::store(r1, data.r1);

            skin[i][0] = 1;
            skin[i][1] = sdef_bone.size();
            sdef_bone.push_back(data);
            break;
        }
        case PMX_VERTEX_TYPE_QDEF: {
            read_index(fin, m_header.bone_index_size);
            read_index(fin, m_header.bone_index_size);
            read_index(fin, m_header.bone_index_size);
            read_index(fin, m_header.bone_index_size);

            float ignore;
            read<float>(fin, ignore);
            read<float>(fin, ignore);
            read<float>(fin, ignore);
            read<float>(fin, ignore);
            break;
        }
        default:
            break;
        }

        read<float>(fin, edge[i]);
    }

    // Make vertex buffer.
    /*m_vertex_buffers.resize(PMX_VERTEX_ATTRIBUTE_NUM_TYPES);
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
    m_vertex_buffers[PMX_VERTEX_ATTRIBUTE_EDGE] =
        graphics::rhi::make_vertex_buffer(edge.data(), edge.size());
    m_vertex_buffers[PMX_VERTEX_ATTRIBUTE_SKIN] = graphics::rhi::make_vertex_buffer(
        skin.data(),
        skin.size(),
        graphics::VERTEX_BUFFER_FLAG_COMPUTE_IN);

    if (!bdef_bone.empty())
    {
        m_vertex_buffers[PMX_VERTEX_ATTRIBUTE_BDEF_BONE] = graphics::rhi::make_vertex_buffer(
            bdef_bone.data(),
            bdef_bone.size(),
            graphics::VERTEX_BUFFER_FLAG_COMPUTE_IN);
    }

    if (!sdef_bone.empty())
    {
        m_vertex_buffers[PMX_VERTEX_ATTRIBUTE_SDEF_BONE] = graphics::rhi::make_vertex_buffer(
            sdef_bone.data(),
            sdef_bone.size(),
            graphics::VERTEX_BUFFER_FLAG_COMPUTE_IN);
    }*/

    std::int32_t index_count;
    read<std::int32_t>(fin, index_count);
    m_mesh.indices.resize(index_count);

    for (std::int32_t i = 0; i < index_count; ++i)
        m_mesh.indices[i] = read_index(fin, m_header.vertex_index_size);

    return true;
}

bool pmx_loader::load_material(std::ifstream& fin, std::string_view root_path)
{
    std::int32_t texture_count;
    read<std::int32_t>(fin, texture_count);
    m_mesh.textures.resize(texture_count);

    for (std::int32_t i = 0; i < texture_count; ++i)
        m_mesh.textures[i] = std::string(root_path) + "/" + read_text(fin);

    std::int32_t material_count;
    read<std::int32_t>(fin, material_count);
    m_mesh.materials.resize(material_count);

    for (pmx_material& material : m_mesh.materials)
    {
        material.name_jp = read_text(fin);
        material.name_en = read_text(fin);

        read<float4>(fin, material.diffuse);
        read<float3>(fin, material.specular);
        read<float>(fin, material.specular_strength);
        read<float3>(fin, material.ambient);
        read<pmx_draw_flag>(fin, material.flag);
        read<float4>(fin, material.edge_color);
        read<float>(fin, material.edge_size);
        material.texture_index = read_index(fin, m_header.texture_index_size);
        material.sphere_index = read_index(fin, m_header.texture_index_size);

        read<pmx_sphere_mode>(fin, material.sphere_mode);
        read<pmx_toon_mode>(fin, material.toon_mode);

        if (material.toon_mode == PMX_TOON_MODE_TEXTURE)
        {
            material.toon_index = read_index(fin, m_header.texture_index_size);
        }
        else if (material.toon_mode == PMX_TOON_MODE_INTERNAL)
        {
            std::uint8_t toon_index;
            read<std::uint8_t>(fin, toon_index);
            material.toon_index = toon_index;
        }
        else
        {
            return false;
        }

        material.meta_data = read_text(fin);
        read<std::int32_t>(fin, material.index_count);
    }

    std::size_t offset = 0;
    m_mesh.submeshes.resize(m_mesh.materials.size());
    for (std::size_t i = 0; i < m_mesh.materials.size(); ++i)
    {
        m_mesh.submeshes[i].index_start = offset;
        m_mesh.submeshes[i].index_count = m_mesh.materials[i].index_count;
        m_mesh.submeshes[i].material_index = i;

        offset += m_mesh.materials[i].index_count;
    }

    return true;
}

bool pmx_loader::load_bone(std::ifstream& fin)
{
    std::int32_t bone_count;
    read<std::int32_t>(fin, bone_count);
    m_mesh.bones.resize(bone_count);

    for (pmx_bone& bone : m_mesh.bones)
    {
        bone.name_jp = read_text(fin);
        bone.name_en = read_text(fin);

        read<float3>(fin, bone.position);
        bone.parent_index = read_index(fin, m_header.bone_index_size);
        read<std::int32_t>(fin, bone.layer);

        read<pmx_bone_flag>(fin, bone.flags);

        if (bone.flags & PMX_BONE_FLAG_INDEXED_TAIL_POSITION)
            bone.tail_index = read_index(fin, m_header.bone_index_size);
        else
            read<float3>(fin, bone.tail_position);

        if (bone.flags & PMX_BONE_FLAG_INHERIT_ROTATION ||
            bone.flags & PMX_BONE_FLAG_INHERIT_TRANSLATION)
        {
            bone.inherit_index = read_index(fin, m_header.bone_index_size);
            read<float>(fin, bone.inherit_weight);
        }

        if (bone.flags & PMX_BONE_FLAG_FIXED_AXIS)
            read<float3>(fin, bone.fixed_axis);

        if (bone.flags & PMX_BONE_FLAG_LOCAL_AXIS)
        {
            read<float3>(fin, bone.local_x_axis);
            read<float3>(fin, bone.local_z_axis);
        }

        if (bone.flags & PMX_BONE_FLAG_EXTERNAL_PARENT_DEFORM)
            bone.external_parent_index = read_index(fin, m_header.bone_index_size);

        if (bone.flags & PMX_BONE_FLAG_IK)
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
                    read<float3>(fin, link.limit_min);
                    read<float3>(fin, link.limit_max);
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
    m_mesh.morphs.resize(morph_count);

    for (pmx_morph& morph : m_mesh.morphs)
    {
        morph.name_jp = read_text(fin);
        morph.name_en = read_text(fin);

        read<std::uint8_t>(fin, morph.control_panel);
        read<pmx_morph_type>(fin, morph.type);

        std::int32_t count;
        read<std::int32_t>(fin, count);

        switch (morph.type)
        {
        case PMX_MORPH_TYPE_GROUP: {
            morph.group_morphs.resize(count);
            for (auto& group : morph.group_morphs)
            {
                group.index = read_index(fin, m_header.morph_index_size);
                read<float>(fin, group.weight);
            }
            break;
        }
        case PMX_MORPH_TYPE_VERTEX: {
            morph.vertex_morphs.resize(count);
            for (auto& vertex_morph : morph.vertex_morphs)
            {
                vertex_morph.index = read_index(fin, m_header.vertex_index_size);
                read<float3>(fin, vertex_morph.translation);
            }
            break;
        }
        case PMX_MORPH_TYPE_BONE: {
            morph.bone_morphs.resize(count);
            for (auto& bone_morph : morph.bone_morphs)
            {
                bone_morph.index = read_index(fin, m_header.bone_index_size);
                read<float3>(fin, bone_morph.translation);
                read<float4>(fin, bone_morph.rotation);
            }
            break;
        }
        case PMX_MORPH_TYPE_UV:
        case PMX_MORPH_TYPE_UV_EXT_1:
        case PMX_MORPH_TYPE_UV_EXT_2:
        case PMX_MORPH_TYPE_UV_EXT_3:
        case PMX_MORPH_TYPE_UV_EXT_4: {
            morph.uv_morphs.resize(count);
            for (auto& uv_morph : morph.uv_morphs)
            {
                uv_morph.index = read_index(fin, m_header.vertex_index_size);
                read<float4>(fin, uv_morph.uv);
            }
            break;
        }
        case PMX_MORPH_TYPE_MATERIAL: {
            morph.material_morphs.resize(count);
            for (auto& material_morph : morph.material_morphs)
            {
                material_morph.index = read_index(fin, m_header.material_index_size);
                read<std::uint8_t>(fin, material_morph.operate);
                read<float4>(fin, material_morph.diffuse);
                read<float3>(fin, material_morph.specular);
                read<float>(fin, material_morph.specular_strength);
                read<float3>(fin, material_morph.ambient);
                read<float4>(fin, material_morph.edge_color);
                read<float>(fin, material_morph.edge_scale);
                read<float4>(fin, material_morph.tex_tint);
                read<float4>(fin, material_morph.spa_tint);
                read<float4>(fin, material_morph.toon_tint);
            }
            break;
        }
        case PMX_MORPH_TYPE_FLIP: {
            morph.flip_morphs.resize(count);
            for (auto& flip_morph : morph.flip_morphs)
            {
                flip_morph.index = read_index(fin, m_header.morph_index_size);
                read<float>(fin, flip_morph.weight);
            }
            break;
        }
        case PMX_MORPH_TYPE_IMPULSE: {
            morph.impulse_morphs.resize(count);
            for (auto& impulse_morph : morph.impulse_morphs)
            {
                impulse_morph.index = read_index(fin, m_header.rigidbody_index_size);
                read<std::uint8_t>(fin, impulse_morph.local_flag);
                read<float3>(fin, impulse_morph.translate_velocity);
                read<float3>(fin, impulse_morph.rotate_torque);
            }
            break;
        }
        default:
            return false;
        }
    }

    return true;
}

bool pmx_loader::load_display(std::ifstream& fin)
{
    std::int32_t display_count;
    read<std::int32_t>(fin, display_count);
    m_mesh.display.resize(display_count);

    for (pmx_display_data& display_frame : m_mesh.display)
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
            case PMX_FRAME_TYPE_BONE:
                frame.index = read_index(fin, m_header.bone_index_size);
                break;
            case PMX_FRAME_TYPE_MORPH:
                frame.index = read_index(fin, m_header.morph_index_size);
                break;
            default:
                return false;
            }
        }
    }

    return true;
}

bool pmx_loader::load_physics(std::ifstream& fin)
{

    std::int32_t rigidbody_count;
    read<std::int32_t>(fin, rigidbody_count);
    m_mesh.rigidbodies.resize(rigidbody_count);

    for (pmx_rigidbody& rigidbody : m_mesh.rigidbodies)
    {
        rigidbody.name_jp = read_text(fin);
        rigidbody.name_en = read_text(fin);

        rigidbody.bone_index = read_index(fin, m_header.bone_index_size);
        read<std::uint8_t>(fin, rigidbody.group);
        read<std::uint16_t>(fin, rigidbody.collision_group);

        read<pmx_rigidbody_shape_type>(fin, rigidbody.shape);
        read<float3>(fin, rigidbody.size);

        read<float3>(fin, rigidbody.translate);

        float3 rotate;
        read<float3>(fin, rotate);
        rigidbody.rotate = quaternion::rotation_euler(rotate);

        read<float>(fin, rigidbody.mass);
        read<float>(fin, rigidbody.linear_damping);
        read<float>(fin, rigidbody.angular_damping);
        read<float>(fin, rigidbody.repulsion);
        read<float>(fin, rigidbody.friction);
        read<pmx_rigidbody_mode>(fin, rigidbody.mode);
    }

    std::int32_t joint_count;
    read<std::int32_t>(fin, joint_count);
    m_mesh.joints.resize(joint_count);

    for (pmx_joint& joint : m_mesh.joints)
    {
        joint.name_jp = read_text(fin);
        joint.name_en = read_text(fin);

        read<pmx_joint_type>(fin, joint.type);
        joint.rigidbody_a_index = read_index(fin, m_header.rigidbody_index_size);
        joint.rigidbody_b_index = read_index(fin, m_header.rigidbody_index_size);

        read<float3>(fin, joint.translate);
        float3 rotate;
        read<float3>(fin, rotate);
        joint.rotate = quaternion::rotation_euler(rotate[1], rotate[0], rotate[2]);

        read<float3>(fin, joint.translate_min);
        read<float3>(fin, joint.translate_max);
        read<float3>(fin, joint.rotate_min);
        read<float3>(fin, joint.rotate_max);

        read<float3>(fin, joint.spring_translate_factor);
        read<float3>(fin, joint.spring_rotate_factor);
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
            convert<ENCODE_TYPE_UTF16, ENCODE_TYPE_UTF8>(str, result);
        }
        return result;
    }
    else
    {
        return "";
    }
}
} // namespace violet::sample