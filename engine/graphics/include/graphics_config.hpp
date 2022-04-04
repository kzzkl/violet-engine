#pragma once

#include "dictionary.hpp"
#include "graphics_exports.hpp"
#include "graphics_interface.hpp"
#include <map>
#include <string_view>
#include <vector>

namespace ash::graphics
{
class GRAPHICS_API graphics_config
{
public:
    struct vertex_attribute_config
    {
        std::string name;
        vertex_attribute_type type;
        std::uint32_t index;
    };
    using vertex_layout_config = std::vector<vertex_attribute_config>;

    struct material_attribute_config
    {
        std::string name;
        pipeline_parameter_type type;
        std::size_t size;
    };
    using material_layout_config = std::vector<material_attribute_config>;

    struct pipeline_config
    {
        std::string name;
        std::string vertex_layout;

        std::vector<std::string> pass_parameters;
        std::vector<std::string> unit_parameters;

        std::string vertex_shader;
        std::string pixel_shader;

        primitive_topology_type primitive_topology;
    };
    template <typename T>
    struct config_type;

    template <typename T>
    using config_type_t = config_type<T>::type;

    template <>
    struct config_type<pipeline_parameter_desc>
    {
        using type = material_layout_config;
    };

    template <>
    struct config_type<pipeline_desc>
    {
        using type = pipeline_config;
    };

    template <typename T>
    using find_result = std::tuple<bool, T, const config_type_t<T>*>;

public:
    graphics_config();

    void load(const dictionary& config);

    template <typename T>
    find_result<T> find_desc(std::string_view name);

    template <>
    find_result<pipeline_parameter_desc> find_desc(std::string_view name);

    template <>
    find_result<pipeline_desc> find_desc(std::string_view name);

    inline std::size_t render_concurrency() const noexcept { return m_render_concurrency; }
    inline std::size_t frame_resource() const noexcept { return m_frame_resource; }
    inline std::size_t multiple_sampling() const noexcept { return m_multiple_sampling; }

    inline std::string_view plugin() const noexcept { return m_plugin; }

private:
    void load_vertex_layout(const dictionary& doc);
    void load_material_layout(const dictionary& doc);
    void load_pipeline(const dictionary& doc);

    std::size_t m_render_concurrency;
    std::size_t m_frame_resource;
    std::size_t m_multiple_sampling;

    std::string m_plugin;

    std::map<std::string, vertex_layout_config> m_vertex_layout;
    std::map<std::string, material_layout_config> m_material_layout;
    std::map<std::string, pipeline_config> m_pipeline;

    std::map<std::string, vertex_attribute_type> m_vertex_attribute_map;
    std::map<std::string, pipeline_parameter_type> m_parameter_type_map;
};
} // namespace ash::graphics