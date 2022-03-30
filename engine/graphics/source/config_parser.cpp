#include "config_parser.hpp"
#include "log.hpp"

namespace ash::graphics
{
config_parser::config_parser()
{
    m_vertex_attribute_map["int"] = vertex_attribute_type::INT;
    m_vertex_attribute_map["int2"] = vertex_attribute_type::INT2;
    m_vertex_attribute_map["int3"] = vertex_attribute_type::INT3;
    m_vertex_attribute_map["int4"] = vertex_attribute_type::INT4;
    m_vertex_attribute_map["uint"] = vertex_attribute_type::UINT;
    m_vertex_attribute_map["uint2"] = vertex_attribute_type::UINT2;
    m_vertex_attribute_map["uint3"] = vertex_attribute_type::UINT3;
    m_vertex_attribute_map["uint4"] = vertex_attribute_type::UINT4;
    m_vertex_attribute_map["float"] = vertex_attribute_type::FLOAT;
    m_vertex_attribute_map["float2"] = vertex_attribute_type::FLOAT2;
    m_vertex_attribute_map["float3"] = vertex_attribute_type::FLOAT3;
    m_vertex_attribute_map["float4"] = vertex_attribute_type::FLOAT4;

    m_parameter_type_map["uint"] = pipeline_parameter_type::UINT;
    m_parameter_type_map["float"] = pipeline_parameter_type::FLOAT;
    m_parameter_type_map["float2"] = pipeline_parameter_type::FLOAT2;
    m_parameter_type_map["float3"] = pipeline_parameter_type::FLOAT3;
    m_parameter_type_map["float4"] = pipeline_parameter_type::FLOAT4;
    m_parameter_type_map["float4x4"] = pipeline_parameter_type::FLOAT4x4;
    m_parameter_type_map["float4x4 array"] = pipeline_parameter_type::FLOAT4x4_ARRAY;
    m_parameter_type_map["texture"] = pipeline_parameter_type::TEXTURE;
}

void config_parser::load(const dictionary& config)
{
    load_vertex_layout(config);
    load_material_layout(config);
    load_pipeline(config);

    m_render_concurrency = config["render_concurrency"];
    m_frame_resource = config["frame_resource"];
    m_multiple_sampling = config["multiple_sampling"];

    m_plugin = config["plugin"];
}

template <>
config_parser::find_result<pipeline_parameter_desc> config_parser::find_desc(std::string_view name)
{
    find_result<pipeline_parameter_desc> result = {};

    auto iter = m_material_layout.find(name.data());
    if (iter == m_material_layout.end())
    {
        std::get<0>(result) = false;
    }
    else
    {
        std::get<0>(result) = true;
        std::get<2>(result) = &iter->second;

        auto& desc = std::get<1>(result);
        for (std::size_t i = 0; i < iter->second.size(); ++i)
        {
            desc.data[i].type = iter->second[i].type;
            desc.data[i].size = iter->second[i].size;
        }
        desc.size = iter->second.size();
    }

    return result;
}

template <>
config_parser::find_result<pipeline_desc> config_parser::find_desc(std::string_view name)
{
    find_result<pipeline_desc> result = {};

    auto iter = m_pipeline.find(name.data());
    if (iter == m_pipeline.end())
    {
        std::get<0>(result) = false;
    }
    else
    {
        std::get<0>(result) = true;
        std::get<2>(result) = &iter->second;

        auto& desc = std::get<1>(result);
        std::memcpy(desc.name, name.data(), name.size());

        auto& vertex_layout = m_vertex_layout[iter->second.vertex_layout];
        for (std::size_t i = 0; i < vertex_layout.size(); ++i)
        {
            std::memcpy(
                &desc.vertex_layout.data[i].name,
                vertex_layout[i].name.c_str(),
                vertex_layout[i].name.size());
            desc.vertex_layout.data[i].type = vertex_layout[i].type;
            desc.vertex_layout.data[i].index = vertex_layout[i].index;
        }
        desc.vertex_layout.size = vertex_layout.size();

        std::memcpy(
            desc.vertex_shader,
            iter->second.vertex_shader.c_str(),
            iter->second.vertex_shader.size());
        std::memcpy(
            desc.pixel_shader,
            iter->second.pixel_shader.c_str(),
            iter->second.pixel_shader.size());

        desc.primitive_topology = iter->second.primitive_topology;
    }

    return result;
}

void config_parser::load_vertex_layout(const dictionary& doc)
{
    auto iter = doc.find("vertex_layout");
    if (iter == doc.end())
        return;

    for (auto& [key, value] : iter->items())
    {
        vertex_layout_config& layout = m_vertex_layout[key];
        for (auto& attribute_config : value["attribute"])
        {
            layout.push_back(vertex_attribute_config{
                attribute_config["name"],
                m_vertex_attribute_map[attribute_config["type"]],
                attribute_config["index"]});
        }
    }
}

void config_parser::load_material_layout(const dictionary& doc)
{
    auto iter = doc.find("pipeline_parameter_layout");
    if (iter == doc.end())
        return;

    for (auto& [name, material] : iter->items())
    {
        material_layout_config& param = m_material_layout[name];
        for (auto& attribute : material)
        {
            std::size_t size = 1;
            if (attribute.find("size") != attribute.end())
                size = attribute["size"];

            param.push_back(material_attribute_config{
                attribute["name"],
                m_parameter_type_map[attribute["type"]],
                size});
        }
    }
}

void config_parser::load_pipeline(const dictionary& doc)
{
    auto iter = doc.find("pipeline");
    if (iter == doc.end())
        return;

    for (auto& [key, value] : iter->items())
    {
        pipeline_config& p = m_pipeline[key];
        p.vertex_layout = value["vertex_layout"];

        p.object_parameter = value["object_parameter"];
        p.pass_parameter = value["pass_parameter"];
        p.material_parameter = value["material_parameter"];

        p.vertex_shader = value["vertex_shader"];
        p.pixel_shader = value["pixel_shader"];

        std::string primitive_topology = value["primitive_topology"];
        if (primitive_topology == "line_list")
            p.primitive_topology = primitive_topology_type::LINE_LIST;
        else if (primitive_topology == "triangle_list")
            p.primitive_topology = primitive_topology_type::TRIANGLE_LIST;
    }
}
} // namespace ash::graphics