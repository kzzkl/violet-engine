#pragma once

#include <cstddef>
#include <vector>

namespace violet::d3d12
{
class d3d12_frame_counter
{
public:
    static void initialize(std::size_t frame_counter, std::size_t frame_resousrce_count) noexcept;

    static void tick() noexcept;

    inline static std::size_t frame_counter() noexcept { return instance().m_frame_counter; }
    inline static std::size_t frame_resource_count() noexcept
    {
        return instance().m_frame_resousrce_count;
    }
    inline static std::size_t frame_resource_index() noexcept
    {
        auto& context = instance();
        return context.m_frame_counter % context.m_frame_resousrce_count;
    }

private:
    d3d12_frame_counter() noexcept;
    static d3d12_frame_counter& instance() noexcept;

    std::size_t m_frame_counter;
    std::size_t m_frame_resousrce_count;
};

template <typename T>
class d3d12_frame_resource
{
public:
    using resource_type = T;
    using resource_list = std::vector<resource_type>;

    using iterator = resource_list::iterator;

public:
    d3d12_frame_resource() noexcept : m_resources(d3d12_frame_counter::frame_resource_count()) {}
    d3d12_frame_resource(const T& value)
        : m_resources(d3d12_frame_counter::frame_resource_count(), value)
    {
    }
    d3d12_frame_resource(resource_list&& list) noexcept : m_resources(std::move(list)) {}

    resource_type& get()
    {
        std::size_t index = d3d12_frame_counter::frame_resource_index();
        return m_resources[index];
    }

    iterator begin() noexcept { return m_resources.begin(); }
    iterator end() noexcept { return m_resources.end(); }

    std::size_t size() const noexcept { return m_resources.size(); }

    resource_type& operator[](std::size_t index) { return m_resources[index]; }

private:
    resource_list m_resources;
};
} // namespace violet::d3d12