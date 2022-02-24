#include "task_manager.hpp"
#include "log.hpp"

using namespace ash::common;

namespace ash::task
{
class root_task : public task
{
public:
    root_task();
    virtual void execute() override;
};

root_task::root_task() : task("root")
{
}

void root_task::execute()
{
    log::debug("root");
}

task_handle::task_handle() : task_handle(nullptr, 0)
{
}

task_handle::task_handle(task_manager* owner, std::size_t index) : m_owner(owner), m_index(index)
{
}

task* task_handle::operator->()
{
    return m_owner->m_tasks[m_index].get();
}

task_manager::task_manager(std::size_t num_thread) : m_thread_pool(num_thread)
{
    m_root = schedule_task<root_task>();
}

void task_manager::run()
{
    m_thread_pool.run(m_queue);
    m_queue.push(&(*m_root));
}

void task_manager::stop()
{
    m_thread_pool.stop();
}

task_manager::handle task_manager::get_root()
{
    return m_root;
}
} // namespace ash::task