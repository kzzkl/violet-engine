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
    m_frame_resource = config["frame_resource"];

    m_plugin = config["plugin"];
}

template <>
config_parser::find_result<pipeline_parameter_desc> config_parser::find_desc(std::string_view name)
{
    find_result<pipeline_parameter_desc> result = {};

    auto iter = m_parameter.find(name.data());
    if (iter == m_parameter.end())
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
            std::memcpy(
                desc.data[i].name,
                iter->second[i].first.c_str(),
                iter->second[i].first.size());
            desc.data[i].type = iter->second[i].second;
        }
        desc.size = iter->second.size();
    }

    return result;
}

template <>
config_parser::find_result<pipeline_layout_desc> config_parser::find_desc(std::string_view name)
{
    find_result<pipeline_layout_desc> result = {};

    auto iter = m_parameter_layout.find(name.data());
    if (iter == m_parameter_layout.end())
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
            auto [found, parameter_desc, _] = find_desc<pipeline_parameter_desc>(iter->second[i]);
            if (found)
            {
                std::memcpy(&desc.data[i], &parameter_desc, sizeof(pipeline_parameter_desc));
            }
            else
            {
                std::get<0>(result) = false;
                return result;
            }
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
    }

    return result;
}

void config_parser::load_vertex_layout(const dictionary& doc)
{
    auto iter = doc.find("vertex_layout");
    if (iter == doc.end())
        return;

    for (auto& layout_config : *iter)
    {
        vertex_layout_config& layout = m_vertex_layout[layout_config["name"]];
        for (auto& attribute_config : layout_config["attribute"])
        {
            layout.push_back(
                vertex_attribute_config{attribute_config["name"],
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

    for (auto parameter_iter = iter->begin(); parameter_iter != iter->end(); ++parameter_iter)
    {
        pipeline_parameter_config& param = m_parameter[parameter_iter.key()];

        auto& type = (*parameter_iter)["type"];
        if (type.is_object())
        {
            for (auto field_iter = type.begin(); field_iter != type.end(); ++field_iter)
            {
                std::string n = field_iter.key();
                pipeline_parameter_type t = m_parameter_layout_map[field_iter.value()];
                param.push_back(std::make_pair(n, t));
            }
        }
        else
        {
            std::string n = parameter_iter.key();
            pipeline_parameter_type t = m_parameter_layout_map[type];
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
        pipeline_layout_config& layout = m_parameter_layout[layout_config["name"]];
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

    for (auto& config : *iter)
    {
        pipeline_config& p = m_pipeline[config["name"]];
        p.vertex_layout = config["vertex_layout"];
        p.parameter_layout = config["parameter_layout"];
        p.vertex_shader = config["vertex_shader"];
        p.pixel_shader = config["pixel_shader"];
    }
}
} // namespace ash::graphics