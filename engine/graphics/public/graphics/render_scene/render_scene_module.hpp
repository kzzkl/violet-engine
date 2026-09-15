#pragma once

#include "common/type_index.hpp"
#include "graphics/shader.hpp"
#include <memory>
#include <vector>

namespace violet
{
class render_scene_context;
class gpu_buffer_uploader;
class render_scene_module
{
public:
    virtual ~render_scene_module() = default;

    virtual void update(render_scene_context& context, gpu_buffer_uploader& uploader) {}
    virtual void reset() {}
};

class render_scene_context
{
public:
    template <typename T, typename... Args>
    void add_module(Args&&... args)
    {
        std::uint32_t index = render_scene_module_index::value<T>();

        if (m_modules.size() <= index)
        {
            m_modules.resize(index + 1);
        }

        m_modules[index] = std::make_unique<T>(std::forward<Args>(args)...);
    }

    template <typename T>
    T& get_module()
    {
        return *static_cast<T*>(m_modules[render_scene_module_index::value<T>()].get());
    }

    template <typename T>
    const T& get_module() const
    {
        return *static_cast<const T*>(m_modules[render_scene_module_index::value<T>()].get());
    }

    void update(gpu_buffer_uploader& uploader)
    {
        for (auto& module : m_modules)
        {
            module->update(*this, uploader);
        }
    }

    void reset()
    {
        for (auto& module : m_modules)
        {
            module->reset();
        }
    }

    shader::scene_data scene_data;

private:
    struct render_scene_module_index : public type_index<render_scene_module_index>
    {
    };

    std::vector<std::unique_ptr<render_scene_module>> m_modules;

    friend class render_scene;
};
} // namespace violet