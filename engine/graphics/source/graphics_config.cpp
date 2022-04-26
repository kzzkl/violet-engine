#include "graphics_config.hpp"
#include "log.hpp"

namespace ash::graphics
{
graphics_config::graphics_config()
{
}

void graphics_config::load(const dictionary& config)
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
graphics_config::find_result<pipeline_parameter_desc> graphics_config::find_desc(
    std::string_view name)
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
graphics_config::find_result<pipeline_desc> graphics_config::find_desc(std::string_view name)
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
        desc.blend = iter->second.blend;
        desc.depth_stencil = iter->second.depth_stencil;
    }

    return result;
}

void graphics_config::load_vertex_layout(const dictionary& doc)
{
    static std::map<std::string, vertex_attribute_type> attribute_map = {
        {"int",    vertex_attribute_type::INT   },
        {"int2",   vertex_attribute_type::INT2  },
        {"int3",   vertex_attribute_type::INT3  },
        {"int4",   vertex_attribute_type::INT4  },
        {"uint",   vertex_attribute_type::UINT  },
        {"uint2",  vertex_attribute_type::UINT2 },
        {"uint3",  vertex_attribute_type::UINT3 },
        {"uint4",  vertex_attribute_type::UINT4 },
        {"float",  vertex_attribute_type::FLOAT },
        {"float2", vertex_attribute_type::FLOAT2},
        {"float3", vertex_attribute_type::FLOAT3},
        {"float4", vertex_attribute_type::FLOAT4},
        {"color",  vertex_attribute_type::COLOR }
    };

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
                attribute_map[attribute_config["type"]],
                attribute_config["index"]});
        }
    }
}

void graphics_config::load_material_layout(const dictionary& doc)
{
    static std::map<std::string, pipeline_parameter_type> parameter_map = {
        {"uint",           pipeline_parameter_type::UINT          },
        {"float",          pipeline_parameter_type::FLOAT         },
        {"float2",         pipeline_parameter_type::FLOAT2        },
        {"float3",         pipeline_parameter_type::FLOAT3        },
        {"float4",         pipeline_parameter_type::FLOAT4        },
        {"float4x4",       pipeline_parameter_type::FLOAT4x4      },
        {"float4x4 array", pipeline_parameter_type::FLOAT4x4_ARRAY},
        {"texture",        pipeline_parameter_type::TEXTURE       },
    };

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
                parameter_map[attribute["type"]],
                size});
        }
    }
}

void graphics_config::load_pipeline(const dictionary& doc)
{
    static std::map<std::string, pipeline_blend_desc::factor_type> factor_map = {
        {"zero",             pipeline_blend_desc::factor_type::ZERO            },
        {"one",              pipeline_blend_desc::factor_type::ONE             },
        {"source_color",     pipeline_blend_desc::factor_type::SOURCE_COLOR    },
        {"source_alpha",     pipeline_blend_desc::factor_type::SOURCE_ALPHA    },
        {"source_inv_alpha", pipeline_blend_desc::factor_type::SOURCE_INV_ALPHA},
        {"target_color",     pipeline_blend_desc::factor_type::TARGET_COLOR    },
        {"target_alpha",     pipeline_blend_desc::factor_type::TARGET_ALPHA    },
        {"target_inv_alpha", pipeline_blend_desc::factor_type::TARGET_INV_ALPHA}
    };

    static std::map<std::string, pipeline_blend_desc::op_type> op_map = {
        {"add",      pipeline_blend_desc::op_type::ADD     },
        {"subtract", pipeline_blend_desc::op_type::SUBTRACT},
        {"min",      pipeline_blend_desc::op_type::MIN     },
        {"max",      pipeline_blend_desc::op_type::MAX     }
    };

    auto iter = doc.find("pipeline");
    if (iter == doc.end())
        return;

    for (auto& [key, value] : iter->items())
    {
        pipeline_config& p = m_pipeline[key];
        p.vertex_layout = value["vertex_layout"];

        for (auto& pass_parameter : value["parameter"]["pass"])
            p.pass_parameters.push_back(pass_parameter);
        for (auto& unit_parameter : value["parameter"]["unit"])
            p.unit_parameters.push_back(unit_parameter);

        p.vertex_shader = value["vertex_shader"];
        p.pixel_shader = value["pixel_shader"];

        std::string primitive_topology = value["primitive_topology"];
        if (primitive_topology == "line_list")
            p.primitive_topology = primitive_topology_type::LINE_LIST;
        else if (primitive_topology == "triangle_list")
            p.primitive_topology = primitive_topology_type::TRIANGLE_LIST;

        if (value.find("blend") != value.end())
        {
            auto& blend_config = value["blend"];

            p.blend.enable = true;
            p.blend.source_factor = factor_map[blend_config["source_factor"]];
            p.blend.target_factor = factor_map[blend_config["target_factor"]];
            p.blend.op = op_map[blend_config["op"]];
            p.blend.source_alpha_factor = factor_map[blend_config["source_alpha_factor"]];
            p.blend.target_alpha_factor = factor_map[blend_config["target_alpha_factor"]];
            p.blend.alpha_op = op_map[blend_config["alpha_op"]];
        }
        else
        {
            p.blend.enable = false;
        }

        if (value.find("depth_stencil") != value.end())
        {
            auto& depth_stencil_config = value["depth_stencil"];

            if (depth_stencil_config["depth_functor"] == "always")
                p.depth_stencil.depth_functor =
                    pipeline_depth_stencil_desc::depth_functor_type::ALWAYS;
            else
                p.depth_stencil.depth_functor =
                    pipeline_depth_stencil_desc::depth_functor_type::LESS;
        }
        else
        {
            p.depth_stencil.depth_functor = pipeline_depth_stencil_desc::depth_functor_type::LESS;
        }
    }
}
} // namespace ash::graphics