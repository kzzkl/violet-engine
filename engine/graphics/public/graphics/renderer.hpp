#pragma once

#include "graphics/render_graph/render_graph.hpp"

namespace violet
{
struct render_feature_index : public type_index<render_feature_index, std::uint32_t>
{
};

class render_feature_base
{
public:
    virtual ~render_feature_base() = default;

    void update(std::uint32_t width, std::uint32_t height)
    {
        on_update(width, height);
    }

    void enable()
    {
        if (!m_enable)
        {
            m_enable = true;
            on_enable();
        }
    }

    void disable()
    {
        if (m_enable)
        {
            m_enable = false;
            on_disable();
        }
    }

    bool is_enable() const noexcept
    {
        return m_enable;
    }

    virtual std::uint32_t get_id() const noexcept = 0;

private:
    virtual void on_update(std::uint32_t width, std::uint32_t height) {}
    virtual void on_enable() {}
    virtual void on_disable() {}

    bool m_enable{false};
};

template <typename T>
class render_feature : public render_feature_base
{
public:
    std::uint32_t get_id() const noexcept override
    {
        return render_feature_index::value<T>();
    }
};

class renderer
{
public:
    renderer() = default;
    virtual ~renderer() = default;

    void render(render_graph& graph);

    template <typename T>
    T* add_feature()
    {
        std::uint32_t id = render_feature_index::value<T>();
        if (m_features.size() <= id)
        {
            m_features.resize(id + 1);
        }

        if (m_features[id] == nullptr)
        {
            m_features[id] = std::make_unique<T>();
            m_features[id]->enable();
        }

        return static_cast<T*>(m_features[id].get());
    }

    template <typename T>
    void remove_feature()
    {
        std::uint32_t id = render_feature_index::value<T>();
        if (m_features.size() > id)
        {
            m_features[id] = nullptr;
        }
    }

    template <typename T>
    T* get_feature(bool check_enable = false) const noexcept
    {
        std::uint32_t id = render_feature_index::value<T>();

        if (m_features.size() <= id || m_features[id] == nullptr)
        {
            return nullptr;
        }

        if (check_enable && !m_features[id]->is_enable())
        {
            return nullptr;
        }

        return static_cast<T*>(m_features[id].get());
    }

    template <typename T>
    bool is_feature_enabled() const noexcept
    {
        std::uint32_t id = render_feature_index::value<T>();
        return m_features.size() > id && m_features[id] != nullptr && m_features[id]->is_enable();
    }

protected:
    virtual void on_render(render_graph& graph) = 0;

private:
    std::vector<std::unique_ptr<render_feature_base>> m_features;
};
} // namespace violet