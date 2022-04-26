#pragma once

#include "context.hpp"
#include "editor_data.hpp"
#include <memory>

namespace ash::editor
{
class editor_view
{
public:
    editor_view(core::context* context) : m_context(context) {}
    virtual ~editor_view() = default;

    virtual void draw(editor_data& data) = 0;

protected:
    template <typename T>
    T& system() const
    {
        return m_context->system<T>();
    }

    core::context* context() const noexcept { return m_context; }

private:
    core::context* m_context;
};

struct editor_ui
{
    bool show;
    std::unique_ptr<editor_view> interface;
};
} // namespace ash::editor