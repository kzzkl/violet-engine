#pragma once

#include "graphics_interface.hpp"
#include "math.hpp"
#include "type_trait.hpp"
#include <memory>
#include <vector>

namespace ash::graphics
{
struct render_parameter_resource
{
    pipeline_parameter* parameter;
    std::vector<resource*> data;
};

template <typename... Types>
class render_parameter
{
public:
    using data_type = std::tuple<Types...>;
    static constexpr std::size_t data_part_count = sizeof...(Types);

public:
    render_parameter(const std::vector<render_parameter_resource>& resources)
        : m_index(0),
          m_dirty(resources.size()),
          m_resources(resources.size())
    {
        for (std::size_t i = 0; i < resources.size(); ++i)
        {
            m_resources[i].parameter.reset(resources[i].parameter);
            m_resources[i].data.resize(resources[i].data.size());

            for (std::size_t j = 0; j < resources[i].data.size(); ++j)
            {
                m_resources[i].data[j].reset(resources[i].data[j]);
                m_resources[i].parameter->bind(j, m_resources[i].data[j].get());
            }
        }
    }

    template <std::size_t index, typename T>
    void set(const T& data) noexcept
    {
        std::get<index>(m_data) = data;
        m_dirty[index] = m_resources.size();
    }

    void sync_resource()
    {
        m_index = (m_index + 1) % m_resources.size();

        type_list<Types...>::each([this]<typename T>() {
            static constexpr std::size_t index = type_list<Types...>::template index<T>();
            if (m_dirty[index] > 0)
            {
                m_resources[m_index].data[index]->upload(
                    &std::get<index>(m_data),
                    sizeof(std::get<index>(m_data)));
                --m_dirty[index];
            }
        });
    }

    pipeline_parameter* parameter() { return m_resources[m_index].parameter.get(); }

private:
    struct frame_resource
    {
        std::unique_ptr<pipeline_parameter> parameter;
        std::vector<std::unique_ptr<resource>> data;
    };

    std::size_t m_index;

    data_type m_data;
    std::vector<std::size_t> m_dirty;
    std::vector<frame_resource> m_resources;
};

struct render_pass_data
{
    math::float4 camera_position;
    math::float4 camera_direction;

    math::float4x4 camera_view;
    math::float4x4 camera_projection;
    math::float4x4 camera_view_projection;
};
using render_parameter_pass = render_parameter<render_pass_data>;

struct render_object_data
{
    math::float4x4 to_world;
};

using render_parameter_object = render_parameter<render_object_data>;
} // namespace ash::graphics