#include "task/task_scheduler.hpp"
#include "task_allocator.hpp"
#include "task_queue.hpp"
#include <cassert>

namespace violet
{
namespace
{
constexpr std::uint32_t TASK_MAIN_QUEUE_SIZE = 1024;
constexpr std::uint32_t TASK_GLOBAL_QUEUE_SIZE = 8 * 1024;
constexpr std::uint32_t TASK_WORK_QUEUE_SIZE = 1024;
constexpr std::uint32_t TASK_IO_QUEUE_SIZE = 1024;
} // namespace

task_handle::task_handle(const task_handle& other) noexcept
{
    m_block = other.m_block;

    if (m_block != nullptr)
    {
        m_block->reference_count.fetch_add(1, std::memory_order_relaxed);
    }
}

task_handle::task_handle(task_handle&& other) noexcept
{
    m_block = other.m_block;
    other.m_block = nullptr;
}

task_handle::~task_handle()
{
    release();
}

task_handle& task_handle::operator=(const task_handle& other) noexcept
{
    if (this != &other)
    {
        release();

        m_block = other.m_block;

        if (m_block != nullptr)
        {
            m_block->reference_count.fetch_add(1, std::memory_order_relaxed);
        }
    }

    return *this;
}

task_handle& task_handle::operator=(task_handle&& other) noexcept
{
    if (m_block != nullptr)
    {
        release();
    }

    m_block = other.m_block;
    other.m_block = nullptr;

    return *this;
}

bool task_handle::is_valid() const noexcept
{
    return m_block != nullptr;
}

bool task_handle::is_complete() const noexcept
{
    return m_block != nullptr &&
           m_block->state.load(std::memory_order_acquire) == TASK_STATE_COMPLETE;
}

void task_handle::wait() const
{
    if (is_valid())
    {
        m_block->scheduler->wait(m_block);
    }
}

void task_handle::wait_for(std::chrono::milliseconds timeout) const
{
    std::abort();
}

void task_handle::cancel() const
{
    std::abort();
}

void task_handle::push_continuation(task_block* continuation) const
{
    task_block* head = m_block->continuation_head.load(std::memory_order_relaxed);
    do
    {
        continuation->dispatch_next = head;
    } while (!m_block->continuation_head.compare_exchange_weak(
        head,
        continuation,
        std::memory_order_acq_rel,
        std::memory_order_relaxed));
}

void task_handle::release()
{
    if (m_block != nullptr)
    {
        if (m_block->reference_count.fetch_sub(1, std::memory_order_acq_rel) == 1)
        {
            m_block->scheduler->free_block(m_block);
        }

        m_block = nullptr;
    }
}

task_event::task_event(std::uint32_t count)
    : m_count(count)
{
}

task_event::~task_event()
{
    // The owner must guarantee that no thread is inside wait() / then() when the event is
    // destroyed. The destructor's lock only serialises against an in-flight notify().
    std::scoped_lock lock(m_mutex);

    assert(m_count.load(std::memory_order_relaxed) == 0);

    if (m_latch != nullptr)
    {
        if (m_latch->reference_count.fetch_sub(1, std::memory_order_acq_rel) == 1)
        {
            m_latch->scheduler->free_block(m_latch);
        }
    }
}

void task_event::add(std::uint32_t count)
{
    if (count == 0)
    {
        return;
    }

    assert(!m_fired.load(std::memory_order_relaxed) && "add() after the event fired");

    m_count.fetch_add(count, std::memory_order_relaxed);
}

void task_event::notify()
{
    const std::uint32_t previous = m_count.fetch_sub(1, std::memory_order_acq_rel);
    assert(previous > 0 && "notify() without a matching add()");

    if (previous != 1)
    {
        return;
    }

    std::vector<task_block*> waiters;
    {
        std::scoped_lock lock(m_mutex);
        m_fired.store(true, std::memory_order_release);
        waiters.swap(m_waiters);

        if (m_latch != nullptr)
        {
            m_latch->state.store(TASK_STATE_COMPLETE, std::memory_order_release);
            m_latch->state.notify_all();
        }
    }

    for (task_block* block : waiters)
    {
        block->scheduler->enqueue(block);
    }
}

void task_event::wait()
{
    task_scheduler::instance().wait(latch_block());
}

void task_event::reset()
{
    assert(m_count.load(std::memory_order_relaxed) == 0);
    assert(m_waiters.empty());

    std::scoped_lock lock(m_mutex);

    m_count.store(0, std::memory_order_relaxed);
    m_fired.store(false, std::memory_order_release);

    if (m_latch != nullptr)
    {
        m_latch->state.store(TASK_STATE_WAITING, std::memory_order_relaxed);
    }
}

task_block* task_event::latch_block()
{
    std::lock_guard lock(m_mutex);

    if (m_latch == nullptr)
    {
        m_latch = task_scheduler::instance().allocate_block();
        m_latch->state.store(TASK_STATE_WAITING, std::memory_order_relaxed);
        m_latch->reference_count.store(1, std::memory_order_relaxed);
        m_latch->scheduler = &task_scheduler::instance();
    }

    if (m_fired.load(std::memory_order_acquire))
    {
        m_latch->state.store(TASK_STATE_COMPLETE, std::memory_order_release);
    }

    return m_latch;
}

thread_local task_scheduler::task_worker* task_scheduler::m_current_worker = nullptr;
thread_local task_thread_type task_scheduler::m_thread_type = TASK_THREAD_OTHER;

task_scheduler::task_scheduler() = default;

task_scheduler::~task_scheduler()
{
    stop();

    m_workers.clear();
    m_main_local_allocator.reset();
    m_shared_allocator.reset();
}

task_scheduler& task_scheduler::instance()
{
    static task_scheduler scheduler;
    return scheduler;
}

void task_scheduler::run(std::uint32_t worker_count)
{
    assert(!m_running.load());

    m_thread_type = TASK_THREAD_MAIN;

    if (worker_count == 0)
    {
        worker_count = std::max<std::uint32_t>(std::thread::hardware_concurrency(), 2u) - 1;
    }

    m_workers.clear();
    m_main_local_allocator.reset();

    m_workers.resize(worker_count);
    m_threads.resize(worker_count);

    m_main_queue = std::make_unique<task_block_queue>(TASK_MAIN_QUEUE_SIZE);
    m_global_queue = std::make_unique<task_block_queue>(TASK_GLOBAL_QUEUE_SIZE);

    if (worker_count > 1)
    {
        m_work_queues.resize(worker_count - 1);

        m_io_queue = std::make_unique<task_block_queue>(TASK_IO_QUEUE_SIZE);
        m_io_worker_id = static_cast<std::uint32_t>(m_workers.size() - 1);
    }
    else
    {
        m_work_queues.resize(1);
        m_io_worker_id = 0xFFFFFFFF;
    }

    for (auto& queue : m_work_queues)
    {
        queue = std::make_unique<task_work_queue>(TASK_WORK_QUEUE_SIZE);
    }

    m_running.store(true, std::memory_order_release);

    m_shared_allocator = std::make_unique<task_allocator_shared>();
    m_main_local_allocator = std::make_unique<task_allocator_local>(m_shared_allocator.get());

    for (std::uint32_t i = 0; i < worker_count; ++i)
    {
        auto worker = std::make_unique<task_worker>();
        worker->id = i;
        worker->local_allocator = std::make_unique<task_allocator_local>(m_shared_allocator.get());

        m_workers[i] = std::move(worker);

        m_threads[i] = std::thread(
            [i, this]()
            {
                work_thread(i, i != m_io_worker_id ? m_wake_sequence : m_io_wake_sequence);
            });
    }
}

void task_scheduler::stop()
{
    if (!m_running.load())
    {
        return;
    }

    pump_main(std::numeric_limits<std::size_t>::max(), std::chrono::microseconds(-1));

    m_running.store(false, std::memory_order_release);

    m_wake_sequence.fetch_add(1, std::memory_order_release);
    m_wake_sequence.notify_all();

    m_io_wake_sequence.fetch_add(1, std::memory_order_release);
    m_io_wake_sequence.notify_all();

    for (auto& thread : m_threads)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }
}

std::size_t task_scheduler::pump_main(std::size_t max_count, std::chrono::microseconds budget)
{
    assert(get_thread_type() == TASK_THREAD_MAIN);

    if (m_main_queue == nullptr)
    {
        return 0;
    }

    const auto deadline = std::chrono::steady_clock::now() + budget;

    std::size_t executed = 0;

    while (executed < max_count)
    {
        task_block* block = m_main_queue->try_pop();
        if (block == nullptr)
        {
            break;
        }

        execute(block);
        ++executed;

        if (budget.count() > 0 && std::chrono::steady_clock::now() >= deadline)
        {
            break;
        }
    }

    return executed;
}

void task_scheduler::work_thread(std::uint32_t worker_id, std::atomic<std::uint32_t>& sequence)
{
    m_current_worker = m_workers[worker_id].get();
    m_thread_type = TASK_THREAD_WORKER;

    while (m_running.load(std::memory_order_acquire))
    {
        const std::uint32_t observed = sequence.load(std::memory_order_acquire);

        if (task_block* block = try_pop_work())
        {
            execute(block);
            continue;
        }

        if (!m_running.load(std::memory_order_acquire))
        {
            break;
        }

        park(sequence, observed);
    }
}

void task_scheduler::enqueue(task_block* block)
{
    if (block->affinity == TASK_AFFINITY_BLOCKING)
    {
        if (m_io_queue != nullptr)
        {
            while (!m_io_queue->push(block) && m_running.load(std::memory_order_relaxed))
            {
                std::this_thread::yield();
            }

            notify(m_io_wake_sequence);

            return;
        }
    }
    else if (block->affinity == TASK_AFFINITY_MAIN)
    {
        while (!m_main_queue->push(block) && m_running.load(std::memory_order_relaxed))
        {
            std::this_thread::yield();
        }

        return;
    }
    else if (get_thread_type() == TASK_THREAD_WORKER)
    {
        std::uint32_t worker_id = m_current_worker->id;
        if (worker_id != m_io_worker_id && m_work_queues[worker_id]->push(block))
        {
            return;
        }
    }

    while (!m_global_queue->push(block) && m_running.load(std::memory_order_relaxed))
    {
        if (get_thread_type() == TASK_THREAD_MAIN && pump_main(1, std::chrono::microseconds(0)) > 0)
        {
            continue;
        }

        std::this_thread::yield();
    }

    notify(m_wake_sequence);
}

void task_scheduler::execute(task_block* block)
{
    assert(block != nullptr && block->function);

    block->state.store(TASK_STATE_RUNNING, std::memory_order_release);

    block->function();

    block->state.store(TASK_STATE_COMPLETE, std::memory_order_release);

    dispatch_continuations(block);

    if (block->reference_count.load(std::memory_order_acquire) > 1)
    {
        block->state.notify_all();
    }

    if (block->reference_count.fetch_sub(1, std::memory_order_acq_rel) == 1)
    {
        free_block(block);
    }
}

task_block* task_scheduler::try_pop_work()
{
    std::uint32_t worker_id = m_current_worker->id;

    auto try_steal = [&]() -> task_block*
    {
        std::uint32_t start = m_steal_cursor.fetch_add(1, std::memory_order_relaxed);

        for (std::uint32_t i = 0; i < m_work_queues.size(); ++i)
        {
            std::uint32_t index = (start + i) % m_work_queues.size();
            if (index == worker_id)
            {
                continue;
            }

            if (task_block* block = m_work_queues[index]->try_steal())
            {
                return block;
            }
        }

        return nullptr;
    };

    if (worker_id != m_io_worker_id)
    {
        if (task_block* block = m_work_queues[worker_id]->try_pop())
        {
            return block;
        }

        if (task_block* block = m_global_queue->try_pop())
        {
            return block;
        }

        if (task_block* block = try_steal())
        {
            return block;
        }
    }
    else
    {
        if (task_block* block = m_io_queue->try_pop())
        {
            return block;
        }
    }

    return nullptr;
}

task_block* task_scheduler::try_pop_help_work()
{
    if (task_block* block = m_global_queue->try_pop())
    {
        return block;
    }

    const auto count = static_cast<std::uint32_t>(m_work_queues.size());
    if (count == 0)
    {
        return nullptr;
    }

    const std::uint32_t start = m_steal_cursor.fetch_add(1, std::memory_order_relaxed);

    for (std::uint32_t i = 0; i < count; ++i)
    {
        if (task_block* block = m_work_queues[(start + i) % count]->try_steal())
        {
            return block;
        }
    }

    return nullptr;
}

void task_scheduler::notify(std::atomic<std::uint32_t>& sequence)
{
    sequence.fetch_add(1, std::memory_order_release);
    sequence.notify_one();
}

void task_scheduler::park(std::atomic<std::uint32_t>& sequence, std::uint32_t observed)
{
    for (int i = 0; i < 64; ++i)
    {
        _mm_pause();

        if (sequence.load(std::memory_order_acquire) != observed)
        {
            return;
        }
    }

    for (int i = 0; i < 8; ++i)
    {
        std::this_thread::yield();

        if (sequence.load(std::memory_order_acquire) != observed)
        {
            return;
        }
    }

    sequence.wait(observed, std::memory_order_acquire);
}

void task_scheduler::wait(task_block* block)
{
    assert(block != nullptr);

    task_thread_type thread_type = get_thread_type();

    while (true)
    {
        task_state state = block->state.load(std::memory_order_acquire);

        if (state == TASK_STATE_COMPLETE)
        {
            break;
        }

        if (!m_running.load(std::memory_order_acquire))
        {
            std::abort();
        }

        switch (thread_type)
        {
        case TASK_THREAD_MAIN: {
            if (pump_main(1, std::chrono::microseconds(0)) > 0)
            {
                break;
            }

            if (task_block* work = try_pop_help_work())
            {
                execute(work);
                break;
            }

            std::this_thread::yield();
            break;
        }
        case TASK_THREAD_WORKER: {
            if (task_block* block = try_pop_work())
            {
                execute(block);
            }
            else
            {
                std::this_thread::yield();
            }
            break;
        }
        case TASK_THREAD_OTHER: {
            block->state.wait(state, std::memory_order_acquire);
            break;
        }
        default:
            break;
        }
    }
}

void task_scheduler::dispatch_continuations(task_block* block)
{
    if (task_block* head = block->continuation_head.exchange(nullptr, std::memory_order_acq_rel))
    {
        while (head != nullptr)
        {
            task_block* next = head->dispatch_next;
            head->dispatch_next = nullptr;
            enqueue(head);
            head = next;
        }
    }
}

task_block* task_scheduler::allocate_block()
{
    switch (get_thread_type())
    {
    case TASK_THREAD_MAIN:
        return m_main_local_allocator->allocate();
    case TASK_THREAD_WORKER:
        return m_current_worker->local_allocator->allocate();
    default:
        return m_shared_allocator->allocate();
    }
}

void task_scheduler::free_block(task_block* block)
{
    switch (get_thread_type())
    {
    case TASK_THREAD_MAIN:
        m_main_local_allocator->free(block);
        break;
    case TASK_THREAD_WORKER:
        m_current_worker->local_allocator->free(block);
        break;
    default:
        m_shared_allocator->free(block);
        break;
    }
}
} // namespace violet