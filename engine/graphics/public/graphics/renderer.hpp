#pragma once

#include "common/type_index.hpp"
#include "graphics/render_context.hpp"
#include "graphics/render_graph/render_graph.hpp"

namespace violet
{
class renderer
{
public:
    renderer();
    virtual ~renderer();

    virtual void render(
        render_graph& graph,
        const render_context& context,
        const render_camera& camera) = 0;

protected:
    template <typename T, typename... Args>
    void add_pass(Args&&... args)
    {
        T::render(std::forward<Args>(args)...);
    }
};
} // namespace violet