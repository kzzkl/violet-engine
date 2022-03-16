#include "config_parser.hpp"

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

    m_parameter_layout_map["buffer"] = pipeline_parameter_type::BUFFER;
    m_parameter_layout_map["texture"] = pipeline_parameter_type::TEXTURE;
}

void config_parser::load(const dictionary& config)
{
    load_vertex_layout(config);
    load_parameter(config);
    load_parameter_layout(config);
    load_pipeline(config);

    m_render_concurrency = config["render_concurrency"];
    m_plugin = config["plugin"];
}

template <>
std::pair<bool, pipeline_parameter_desc> config_parser::find_desc(std::string_view name)
{
    std::pair<bool, pipeline_parameter_desc> result = {};

    auto iter = m_parameter.find(name.data());
    if (iter == m_parameter.end())
    {
        result.first = false;
        return result;
    }
    else
    {
        result.first = true;
    }

    auto& desc = result.second;
    for (std::size_t i = 0; i < iter->second.size(); ++i)
    {
        std::memcpy(desc.data[i].name, iter->second[i].first.c_str(), iter->second[i].first.size());
        desc.data[i].type = iter->second[i].second;
    }
    desc.size = iter->second.size();

    return result;
}

template <>
std::pair<bool, pipeline_parameter_layout_desc> config_parser::find_desc(std::string_view name)
{
    std::pair<bool, pipeline_parameter_layout_desc> result = {};

    auto iter = m_parameter_layout.find(name.data());
    if (iter == m_parameter_layout.end())
    {
        result.first = false;
        return result;
    }
    else
    {
        result.first = true;
    }

    auto& desc = result.second;
    for (std::size_t i = 0; i < iter->second.size(); ++i)
    {
        auto [found, parameter_desc] = find_desc<pipeline_parameter_desc>(iter->second[i]);
        std::memcpy(&desc.data[i], &parameter_desc, sizeof(pipeline_parameter_desc));
    }
    desc.size = iter->second.size();

    return result;
}

template <>
std::pair<bool, pipeline_desc> config_parser::find_desc(std::string_view name)
{
    std::pair<bool, pipeline_desc> result = {};

    auto iter = m_pipeline.find(name.data());
    if (iter == m_pipeline.end())
    {
        result.first = false;
        return result;
    }
    else
    {
        result.first = true;
    }

    auto& desc = result.second;
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

    return result;
}

void config_parser::load_vertex_layout(const dictionary& doc)
{
    auto iter = doc.find("vertex_layout");
    if (iter == doc.end())
        return;

    for (auto& layout_config : *iter)
    {
        vertex_layout& layout = m_vertex_layout[layout_config["name"]];
        for (auto& attribute_config : layout_config["attribute"])
        {
            layout.push_back(vertex_attribute{attribute_config["name"],
                                              m_vertex_attribute_map[attribute_config["type"]],
                                              attribute_config["index"]});
        }
    }
}

void config_parser::load_parameter(const dictionary& doc)
{
    auto iter = doc.find("parameter");
    if (iter == doc.end())
        return;

    for (auto& parameter_config : *iter)
    {
        parameter& param = m_parameter[parameter_config["name"]];
        if (parameter_config["type"].is_object())
        {
            for (auto iter = parameter_config["type"].begin(), end = parameter_config["type"].end();
                 iter != end;
                 ++iter)
            {
                std::string n = iter.key();
                pipeline_parameter_type t = m_parameter_layout_map[iter.value()];
                param.push_back(std::make_pair(n, t));
            }
        }
        else
        {
            std::string n = parameter_config["name"];
            pipeline_parameter_type t = m_parameter_layout_map[parameter_config["type"]];
            param.push_back(std::make_pair(n, t));
        }
    }
}

void config_parser::load_parameter_layout(const dictionary& doc)
{
    auto iter = doc.find("parameter_layout");
    if (iter == doc.end())
        return;

    for (auto& layout_config : *iter)
    {
        parameter_layout& layout = m_parameter_layout[layout_config["name"]];
        for (auto& parameter_config : layout_config["parameter"])
        {
            layout.push_back(parameter_config);
        }
    }
}

void config_parser::load_pipeline(const dictionary& doc)
{
    auto iter = doc.find("pipeline");
    if (iter == doc.end())
        return;

    for (auto& pipeline_config : *iter)
    {
        pipeline& pipeline = m_pipeline[pipeline_config["name"]];
        pipeline.vertex_layout = pipeline_config["vertex_layout"];
        pipeline.parameter_layout = pipeline_config["parameter_layout"];
        pipeline.vertex_shader = pipeline_config["vertex_shader"];
        pipeline.pixel_shader = pipeline_config["pixel_shader"];
    }
}
} // namespace ash::graphics