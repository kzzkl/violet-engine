#include "pmx.hpp"
#include "encode.hpp"
#include <fstream>

namespace violet::sample
{
template <typename T>
static void read(std::istream& fin, T& dest)
{
    fin.read(reinterpret_cast<char*>(&dest), sizeof(T));
}

pmx::pmx(std::string_view path) : m_loaded(false)
{
    std::ifstream fin(path.data(), std::ios::binary);
    if (fin.is_open() && load_header(fin) && load_mesh(fin) &&
        load_material(fin, path.substr(0, path.find_last_of('/'))) && load_bone(fin) &&
        load_morph(fin) && load_display(fin) && load_physics(fin))
        m_loaded = true;

    fin.close();
}

bool pmx::load_header(std::ifstream& fin)
{
    char magic[4];
    fin.read(magic, 4);

    read<float>(fin, header.version);

    std::uint8_t ignore;
    read<std::uint8_t>(fin, ignore); // ignore global size
    read<std::uint8_t>(fin, header.text_encoding);
    read<std::uint8_t>(fin, header.num_add_vec4);
    read<std::uint8_t>(fin, header.vertex_index_size);
    read<std::uint8_t>(fin, header.texture_index_size);
    read<std::uint8_t>(fin, header.material_index_size);
    read<std::uint8_t>(fin, header.bone_index_size);
    read<std::uint8_t>(fin, header.morph_index_size);
    read<std::uint8_t>(fin, header.rigidbody_index_size);

    header.name_jp = read_text(fin);
    header.name_en = read_text(fin);
    header.comments_jp = read_text(fin);
    header.comments_en = read_text(fin);

    return true;
}

bool pmx::load_mesh(std::ifstream& fin)
{
    std::int32_t vertex_count;
    read<std::int32_t>(fin, vertex_count);

    position.resize(vertex_count);
    normal.resize(vertex_count);
    uv.resize(vertex_count);
    skin.resize(vertex_count);
    edge.resize(vertex_count);

    std::vector<std::vector<float4>> add_uv(vertex_count);
    for (auto& v : add_uv)
        v.resize(header.num_add_vec4);

    for (std::size_t i = 0; i < vertex_count; ++i)
    {
        read<float3>(fin, position[i]);
        read<float3>(fin, normal[i]);
        read<float2>(fin, uv[i]);

        for (std::uint8_t j = 0; j < header.num_add_vec4; ++j)
            read<float4>(fin, add_uv[i][j]);

        pmx_vertex_type weight_type;
        read<pmx_vertex_type>(fin, weight_type);

        switch (weight_type)
        {
        case PMX_VERTEX_TYPE_BDEF1: {
            bdef_data data = {};
            data.index[0] = read_index(fin, header.bone_index_size);
            data.weight = {1.0f, 0.0f, 0.0f, 0.0f};

            skin[i][0] = 0;
            skin[i][1] = bdef.size();
            bdef.push_back(data);
            break;
        }
        case PMX_VERTEX_TYPE_BDEF2: {
            bdef_data data = {};
            data.index[0] = read_index(fin, header.bone_index_size);
            data.index[1] = read_index(fin, header.bone_index_size);

            read<float>(fin, data.weight[0]);
            data.weight[1] = 1.0f - data.weight[0];

            skin[i][0] = 0;
            skin[i][1] = bdef.size();
            bdef.push_back(data);
            break;
        }
        case PMX_VERTEX_TYPE_BDEF4: {
            bdef_data data = {};
            data.index[0] = read_index(fin, header.bone_index_size);
            data.index[1] = read_index(fin, header.bone_index_size);
            data.index[2] = read_index(fin, header.bone_index_size);
            data.index[3] = read_index(fin, header.bone_index_size);
            read<float>(fin, data.weight[0]);
            read<float>(fin, data.weight[1]);
            read<float>(fin, data.weight[2]);
            read<float>(fin, data.weight[3]);

            skin[i][0] = 0;
            skin[i][1] = bdef.size();
            bdef.push_back(data);
            break;
        }
        case PMX_VERTEX_TYPE_SDEF: {
            sdef_data data = {};
            data.index[0] = read_index(fin, header.bone_index_size);
            data.index[1] = read_index(fin, header.bone_index_size);
            read<float>(fin, data.weight[0]);
            data.weight[1] = 1.0f - data.weight[0];
            read<float3>(fin, data.center);
            read<float3>(fin, data.r0);
            read<float3>(fin, data.r1);

            vector4 center = vector::load(data.center);
            vector4 r0 = vector::load(data.r0);
            vector4 r1 = vector::load(data.r1);
            vector4 rw =
                vector::add(vector::mul(r0, data.weight[0]), vector::mul(r1, data.weight[1]));
            r0 = vector::add(center, r0);
            r0 = vector::sub(r0, rw);
            r0 = vector::add(center, r0);
            r0 = vector::mul(r0, 0.5f);
            r1 = vector::add(center, r1);
            r1 = vector::sub(r1, rw);
            r1 = vector::add(center, r1);
            r1 = vector::mul(r1, 0.5f);

            vector::store(r0, data.r0);
            vector::store(r1, data.r1);

            skin[i][0] = 1;
            skin[i][1] = sdef.size();
            sdef.push_back(data);
            break;
        }
        case PMX_VERTEX_TYPE_QDEF: {
            read_index(fin, header.bone_index_size);
            read_index(fin, header.bone_index_size);
            read_index(fin, header.bone_index_size);
            read_index(fin, header.bone_index_size);

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

    std::int32_t index_count;
    read<std::int32_t>(fin, index_count);
    indices.resize(index_count);

    for (std::int32_t i = 0; i < index_count; ++i)
        indices[i] = read_index(fin, header.vertex_index_size);

    return true;
}

bool pmx::load_material(std::ifstream& fin, std::string_view root_path)
{
    std::int32_t texture_count;
    read<std::int32_t>(fin, texture_count);
    textures.resize(texture_count);

    for (std::int32_t i = 0; i < texture_count; ++i)
        textures[i] = std::string(root_path) + "/" + read_text(fin);

    std::int32_t material_count;
    read<std::int32_t>(fin, material_count);
    materials.resize(material_count);

    for (pmx_material& material : materials)
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
        material.texture_index = read_index(fin, header.texture_index_size);
        material.sphere_index = read_index(fin, header.texture_index_size);

        read<pmx_sphere_mode>(fin, material.sphere_mode);
        read<pmx_toon_mode>(fin, material.toon_mode);

        if (material.toon_mode == PMX_TOON_MODE_TEXTURE)
        {
            material.toon_index = read_index(fin, header.texture_index_size);
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
    submeshes.resize(materials.size());
    for (std::size_t i = 0; i < materials.size(); ++i)
    {
        submeshes[i].index_start = offset;
        submeshes[i].index_count = materials[i].index_count;
        submeshes[i].material_index = i;

        offset += materials[i].index_count;
    }

    return true;
}

bool pmx::load_bone(std::ifstream& fin)
{
    std::int32_t bone_count;
    read<std::int32_t>(fin, bone_count);
    bones.resize(bone_count);

    for (pmx_bone& bone : bones)
    {
        bone.name_jp = read_text(fin);
        bone.name_en = read_text(fin);

        read<float3>(fin, bone.position);
        bone.parent_index = read_index(fin, header.bone_index_size);
        read<std::int32_t>(fin, bone.layer);

        read<pmx_bone_flag>(fin, bone.flags);

        if (bone.flags & PMX_BONE_FLAG_INDEXED_TAIL_POSITION)
            bone.tail_index = read_index(fin, header.bone_index_size);
        else
            read<float3>(fin, bone.tail_position);

        if (bone.flags & PMX_BONE_FLAG_INHERIT_ROTATION ||
            bone.flags & PMX_BONE_FLAG_INHERIT_TRANSLATION)
        {
            bone.inherit_index = read_index(fin, header.bone_index_size);
            read<float>(fin, bone.inherit_weight);
        }
        else
        {
            bone.inherit_index = -1;
        }

        if (bone.flags & PMX_BONE_FLAG_FIXED_AXIS)
            read<float3>(fin, bone.fixed_axis);

        if (bone.flags & PMX_BONE_FLAG_LOCAL_AXIS)
        {
            read<float3>(fin, bone.local_x_axis);
            read<float3>(fin, bone.local_z_axis);
        }

        if (bone.flags & PMX_BONE_FLAG_EXTERNAL_PARENT_DEFORM)
            bone.external_parent_index = read_index(fin, header.bone_index_size);

        if (bone.flags & PMX_BONE_FLAG_IK)
        {
            bone.ik_target_index = read_index(fin, header.bone_index_size);
            read<std::int32_t>(fin, bone.ik_iteration_count);
            read<float>(fin, bone.ik_limit);

            std::int32_t count;
            read<std::int32_t>(fin, count);
            bone.ik_links.resize(count);

            for (pmx_ik_link& link : bone.ik_links)
            {
                link.bone_index = read_index(fin, header.bone_index_size);

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

bool pmx::load_morph(std::ifstream& fin)
{
    std::int32_t morph_count;
    read<std::int32_t>(fin, morph_count);
    morphs.resize(morph_count);

    for (pmx_morph& morph : morphs)
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
                group.index = read_index(fin, header.morph_index_size);
                read<float>(fin, group.weight);
            }
            break;
        }
        case PMX_MORPH_TYPE_VERTEX: {
            morph.vertex_morphs.resize(count);
            for (auto& vertex_morph : morph.vertex_morphs)
            {
                vertex_morph.index = read_index(fin, header.vertex_index_size);
                read<float3>(fin, vertex_morph.translation);
            }
            break;
        }
        case PMX_MORPH_TYPE_BONE: {
            morph.bone_morphs.resize(count);
            for (auto& bone_morph : morph.bone_morphs)
            {
                bone_morph.index = read_index(fin, header.bone_index_size);
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
                uv_morph.index = read_index(fin, header.vertex_index_size);
                read<float4>(fin, uv_morph.uv);
            }
            break;
        }
        case PMX_MORPH_TYPE_MATERIAL: {
            morph.material_morphs.resize(count);
            for (auto& material_morph : morph.material_morphs)
            {
                material_morph.index = read_index(fin, header.material_index_size);
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
                flip_morph.index = read_index(fin, header.morph_index_size);
                read<float>(fin, flip_morph.weight);
            }
            break;
        }
        case PMX_MORPH_TYPE_IMPULSE: {
            morph.impulse_morphs.resize(count);
            for (auto& impulse_morph : morph.impulse_morphs)
            {
                impulse_morph.index = read_index(fin, header.rigidbody_index_size);
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

bool pmx::load_display(std::ifstream& fin)
{
    std::int32_t display_count;
    read<std::int32_t>(fin, display_count);
    display.resize(display_count);

    for (pmx_display_data& display_frame : display)
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
                frame.index = read_index(fin, header.bone_index_size);
                break;
            case PMX_FRAME_TYPE_MORPH:
                frame.index = read_index(fin, header.morph_index_size);
                break;
            default:
                return false;
            }
        }
    }

    return true;
}

bool pmx::load_physics(std::ifstream& fin)
{
    std::int32_t rigidbody_count;
    read<std::int32_t>(fin, rigidbody_count);
    rigidbodies.resize(rigidbody_count);

    for (pmx_rigidbody& rigidbody : rigidbodies)
    {
        rigidbody.name_jp = read_text(fin);
        rigidbody.name_en = read_text(fin);

        rigidbody.bone_index = read_index(fin, header.bone_index_size);
        read<std::uint8_t>(fin, rigidbody.group);
        read<std::uint16_t>(fin, rigidbody.collision_group);

        read<pmx_rigidbody_shape_type>(fin, rigidbody.shape);
        read<float3>(fin, rigidbody.size);

        read<float3>(fin, rigidbody.translate);

        float3 rotate;
        read<float3>(fin, rotate);
        rigidbody.rotate =
            vector::store<float4>(quaternion::from_euler(rotate.x, rotate.y, rotate.z));

        read<float>(fin, rigidbody.mass);
        read<float>(fin, rigidbody.linear_damping);
        read<float>(fin, rigidbody.angular_damping);
        read<float>(fin, rigidbody.repulsion);
        read<float>(fin, rigidbody.friction);
        read<pmx_rigidbody_mode>(fin, rigidbody.mode);
    }

    std::int32_t joint_count;
    read<std::int32_t>(fin, joint_count);
    joints.resize(joint_count);

    for (pmx_joint& joint : joints)
    {
        joint.name_jp = read_text(fin);
        joint.name_en = read_text(fin);

        read<pmx_joint_type>(fin, joint.type);
        joint.rigidbody_a_index = read_index(fin, header.rigidbody_index_size);
        joint.rigidbody_b_index = read_index(fin, header.rigidbody_index_size);

        read<float3>(fin, joint.translate);
        float3 rotate;
        read<float3>(fin, rotate);
        joint.rotate =
            vector::store<float4>(quaternion::from_euler(rotate[1], rotate[0], rotate[2]));

        read<float3>(fin, joint.translate_min);
        read<float3>(fin, joint.translate_max);
        read<float3>(fin, joint.rotate_min);
        read<float3>(fin, joint.rotate_max);

        read<float3>(fin, joint.spring_translate_factor);
        read<float3>(fin, joint.spring_rotate_factor);
    }

    return true;
}

std::int32_t pmx::read_index(std::ifstream& fin, std::uint8_t size)
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

std::string pmx::read_text(std::ifstream& fin)
{
    std::int32_t len;
    read<std::int32_t>(fin, len);
    if (len > 0)
    {
        std::string result;
        if (header.text_encoding == 1)
        {
            result.resize(len);
            fin.read(result.data(), len);
        }
        else if (header.text_encoding == 0)
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