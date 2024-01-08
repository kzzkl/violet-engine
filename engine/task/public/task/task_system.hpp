#pragma once

#include "core/engine_system.hpp"
#include "task/task_executor.hpp"

namespace violet
{
class task_system : public engine_system
{
public:
    task_system() noexcept;

    virtual bool initialize(const dictionary& config) override;
    virtual void update(float delta) override;
    virtual void late_update(float delta) override;
    virtual void shutdown() override;

    task_executor& get_executor() { return *m_executor; }

    task<>& on_frame_begin() { return m_frame_begin.get_root(); }
    task<>& on_frame_end() { return m_frame_end.get_root(); }
    task<float>& on_tick() { return m_tick.get_root(); }

private:
    std::unique_ptr<task_executor> m_executor;

    task_graph<float> m_task_graph;
    std::future<void> m_future;

    task_graph<> m_frame_begin;
    task_graph<> m_frame_end;
    task_graph<float> m_tick;
};
} // namespace violet