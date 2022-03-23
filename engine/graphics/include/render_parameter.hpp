#pragma once

#include "graphics_interface.hpp"
#include "math.hpp"
#include "texture.hpp"
#include "type_trait.hpp"
#include <memory>
#include <vector>

namespace ash::graphics
{
template <typename T>
struct single
{
    using value_type = T;
    static constexpr bool frame_resource = true;

    value_type value;
};

template <typename T>
struct multiple
{
    using value_type = T;
    static constexpr bool frame_resource = false;

    value_type value;
};

class render_parameter_base
{
public:
    render_parameter_base() = default;
    render_parameter_base(const render_parameter_base&) = default;
    virtual ~render_parameter_base() = default;

    virtual pipeline_parameter* parameter() const = 0;
    virtual void sync_resource() = 0;

    render_parameter_base& operator=(const render_parameter_base&) = default;
};

template <typename... Types>
class render_parameter : public render_parameter_base
{
public:
    using data_type = std::tuple<Types...>;

public:
    render_parameter(
        const std::vector<pipeline_parameter*>& parameters,
        const std::vector<resource*>& parts)
        : m_index(0),
          m_dirty(parameters.size())
    {
        for (auto parameter : parameters)
            m_parameters.emplace_back(parameter);

        for (auto part : parts)
            m_parts.emplace_back(part);

        for (std::size_t i = 0; i < m_parameters.size(); ++i)
        {
            type_list<Types...>::each([this, i]<typename T>() {
                using resource_type = T::value_type;
                constexpr std::size_t part_index = type_list<Types...>::template index<T>();

                if constexpr (std::is_same_v<resource_type, texture>)
                {
                    m_parameters[i]->bind(part_index, m_parts[part_index].get());
                }
                else
                {
                    std::size_t offset = 0;
                    if constexpr (T::frame_resource)
                        offset = i * sizeof(resource_type);

                    m_parameters[i]->bind(part_index, m_parts[part_index].get(), offset);
                }
            });
        }
    }

    template <std::size_t index, typename T>
    void set(const T& data) noexcept
    {
        std::get<index>(m_data).value = data;
        m_dirty[index] = m_parameters.size();
    }

    virtual void sync_resource() override
    {
        m_index = (m_index + 1) % m_parameters.size();

        type_list<Types...>::each([this]<typename T>() {
            static constexpr std::size_t index = type_list<Types...>::template index<T>();
            if (m_dirty[index] > 0)
            {
                std::size_t size = sizeof(std::get<index>(m_data));
                m_parts[index]->upload(&std::get<index>(m_data), size, size * index);
                --m_dirty[index];
            }
        });
    }

    virtual pipeline_parameter* parameter() const override { return m_parameters[m_index].get(); }

private:
    std::size_t m_index;

    data_type m_data;
    std::vector<std::size_t> m_dirty;

    std::vector<std::unique_ptr<pipeline_parameter>> m_parameters;
    std::vector<std::unique_ptr<resource>> m_parts;
};

struct render_pass_data
{
    math::float4 camera_position;
    math::float4 camera_direction;

    math::float4x4 camera_view;
    math::float4x4 camera_projection;
    math::float4x4 camera_view_projection;
};
using render_parameter_pass = render_parameter<multiple<render_pass_data>>;

struct render_object_data
{
    math::float4x4 to_world;
};
using render_parameter_object = render_parameter<multiple<render_object_data>>;
} // namespace ash::graphics