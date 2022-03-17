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
    struct vertex_attribute_config
    {
        std::string name;
        vertex_attribute_type type;
        uint32_t index;
    };
    using vertex_layout_config = std::vector<vertex_attribute_config>;

    using pipeline_parameter_config = std::vector<std::pair<std::string, pipeline_parameter_type>>;
    using pipeline_layout_config = std::vector<std::string>;

    struct pipeline_config
    {
        std::string name;
        std::string vertex_layout;
        std::string parameter_layout;

        std::string vertex_shader;
        std::string pixel_shader;
    };
    template <typename T>
    struct config_type;

    template <typename T>
    using config_type_t = config_type<T>::type;

    template <>
    struct config_type<pipeline_parameter_desc>
    {
        using type = pipeline_parameter_config;
    };

    template <>
    struct config_type<pipeline_layout_desc>
    {
        using type = pipeline_layout_config;
    };

    template <>
    struct config_type<pipeline_desc>
    {
        using type = pipeline_config;
    };

    template <typename T>
    using find_result = std::tuple<bool, T, const config_type_t<T>*>;

public:
    config_parser();

    void load(const dictionary& config);

    template <typename T>
    find_result<T> find_desc(std::string_view name);

    inline std::size_t get_render_concurrency() const noexcept { return m_render_concurrency; }
    inline std::string_view get_plugin() const noexcept { return m_plugin; }

private:
    void load_vertex_layout(const dictionary& doc);
    void load_parameter(const dictionary& doc);
    void load_parameter_layout(const dictionary& doc);
    void load_pipeline(const dictionary& doc);

    std::size_t m_render_concurrency;
    std::string m_plugin;

    std::map<std::string, vertex_layout_config> m_vertex_layout;
    std::map<std::string, pipeline_parameter_config> m_parameter;
    std::map<std::string, pipeline_layout_config> m_parameter_layout;
    std::map<std::string, pipeline_config> m_pipeline;

    std::map<std::string, vertex_attribute_type> m_vertex_attribute_map;
    std::map<std::string, pipeline_parameter_type> m_parameter_layout_map;
};

template <>
config_parser::find_result<pipeline_parameter_desc> config_parser::find_desc(std::string_view name);

template <>
config_parser::find_result<pipeline_layout_desc> config_parser::find_desc(std::string_view name);

template <>
config_parser::find_result<pipeline_desc> config_parser::find_desc(std::string_view name);
} // namespace ash::graphics