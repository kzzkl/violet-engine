#pragma once

#include "common/dictionary.hpp"
#include "common/type_index.hpp"
#include "core/ecs/world.hpp"
#include "core/timer.hpp"
#include "task/task_executor.hpp"

namespace violet
{
struct engine_module_index : public type_index<engine_module_index, std::size_t>
{
};

class engine_context;
class engine_module
{
public:
    engine_module(std::string_view name) noexcept;
    virtual ~engine_module() = default;

    virtual bool initialize(const dictionary& config) { return true; }
    virtual void shutdown() {}

    inline const std::string& get_name() const noexcept { return m_name; }

protected:
    template <typename T>
    T& get_module()
    {
        return *static_cast<T*>(get_module(engine_module_index::value<T>()));
    }

    timer& get_timer();
    world& get_world();

    task<>& on_frame_begin() noexcept;
    task<>& on_frame_end() noexcept;
    task<float>& on_tick() noexcept;

    task_executor& get_executor() noexcept;

private:
    friend class engine;

    engine_module* get_module(std::size_t index);

    std::string m_name;
    engine_context* m_context;
};
} // namespace violet