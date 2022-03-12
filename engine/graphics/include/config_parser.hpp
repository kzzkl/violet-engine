#pragma once

#include "dictionary.hpp"
#include "graphics_interface.hpp"
#include <map>
#include <string_view>
#include <vector>

namespace ash::graphics
{
class config_parser
{
public:
    struct vertex_attribute
    {
        std::string name;
        vertex_attribute_type type;
        uint32_t index;
    };
    using vertex_layout = std::vector<vertex_attribute>;

    using parameter = std::vector<std::pair<std::string, pipeline_parameter_type>>;
    using parameter_layout = std::vector<std::string>;

    struct pipeline
    {
        std::string name;
        std::string vertex_layout;
        std::string parameter_layout;

        std::string vertex_shader;
        std::string pixel_shader;
    };

public:
    config_parser();

    void load(const dictionary& config);

    template <typename T>
    std::pair<bool, T> find_desc(std::string_view name);

    inline std::size_t get_render_concurrency() const noexcept { return m_render_concurrency; }

private:
    void load_vertex_layout(const dictionary& doc);
    void load_parameter(const dictionary& doc);
    void load_parameter_layout(const dictionary& doc);
    void load_pipeline(const dictionary& doc);

    dictionary merge_config(const dictionary& config);

    std::size_t m_render_concurrency;

    std::map<std::string, vertex_layout> m_vertex_layout;
    std::map<std::string, parameter> m_parameter;
    std::map<std::string, parameter_layout> m_parameter_layout;
    std::map<std::string, pipeline> m_pipeline;

    std::map<std::string, vertex_attribute_type> m_vertex_attribute_map;
    std::map<std::string, pipeline_parameter_type> m_parameter_layout_map;
};

template <>
std::pair<bool, pipeline_parameter_desc> config_parser::find_desc(std::string_view name);

template <>
std::pair<bool, pipeline_parameter_layout_desc> config_parser::find_desc(std::string_view name);

template <>
std::pair<bool, pipeline_desc> config_parser::find_desc(std::string_view name);
} // namespace ash::graphics