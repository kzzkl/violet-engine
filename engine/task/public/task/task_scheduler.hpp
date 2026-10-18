#pragma once

#include "task/task_block.hpp"
#include <cassert>
#include <chrono>
#include <mutex>
#include <thread>

namespace violet
{
class task_handle
{
public:
    task_handle() noexcept = default;
    task_handle(const task_handle& other) noexcept;
    task_handle(task_handle&& other) noexcept;
    ~task_handle();

    task_handle& operator=(const task_handle& other) noexcept;
    task_handle& operator=(task_handle&& other) noexcept;

    bool is_valid() const noexcept;
    bool is_complete() const noexcept;

    void wait() const;
    void wait_for(std::chrono::milliseconds timeout) const;

    void cancel() const;

    template <typename F>
    task_handle then(F&& fn, task_affinity affinity = TASK_AFFINITY_ANY) const;

private:
    void push_continuation(task_block* continuation) const;

    void release();

    task_block* m_block{nullptr};

    friend class task_scheduler;
    friend class task_event;
};

class task_event
{
public:
    ~task_event();

    void add(const task_handle& handle);
    void add(std::uint32_t count);

    void notify();

    void wait();

    template <typename F>
    task_handle then(F&& fn, task_affinity affinity = TASK_AFFINITY_ANY);

    void reset();

private:
    task_block* latch_block();

    std::atomic<std::uint32_t> m_count{0};

    std::atomic<bool> m_fired{false};

    std::mutex m_mutex;
    std::vector<task_block*> m_waiters;
    task_block* m_latch{nullptr};
};

class task_allocator_shared;
class task_allocator_local;

class task_work_queue;
class task_block_queue;

enum task_thread_type
{
    TASK_THREAD_MAIN,
    TASK_THREAD_WORKER,
    TASK_THREAD_OTHER,
};

class task_scheduler
{
public:
    ~task_scheduler();

    static task_scheduler& instance();

    void run(std::uint32_t worker_count = 0);
    void stop();

    template <typename F>
    task_handle dispatch(F&& fn, task_affinity affinity = TASK_AFFINITY_ANY)
    {
        task_block* block = allocate_block();
        block->state.store(TASK_STATE_WAITING, std::memory_order_relaxed);
        block->reference_count.store(2, std::memory_order_relaxed);
        block->affinity = affinity;
        block->function = std::forward<F>(fn);
        block->scheduler = this;

        enqueue(block);

        task_handle handle;
        handle.m_block = block;

        return handle;
    }

    std::size_t pump_main(
        std::size_t max_count = 64,
        std::chrono::microseconds budget = std::chrono::microseconds(500));

    task_thread_type get_thread_type() const noexcept
    {
        return m_thread_type;
    }

private:
    struct task_worker
    {
        std::uint32_t id;
        std::unique_ptr<task_allocator_local> local_allocator;
    };

    task_scheduler();

    void work_thread(std::uint32_t worker_id, std::atomic<std::uint32_t>& sequence);

    void enqueue(task_block* block);
    void execute(task_block* block);

    task_block* try_pop_work();

    void notify(std::atomic<std::uint32_t>& sequence);
    void park(std::atomic<std::uint32_t>& sequence, std::uint32_t observed);

    void wait(task_block* block);
    void dispatch_continuations(task_block* block);

    task_block* allocate_block();
    void free_block(task_block* block);

    std::vector<std::unique_ptr<task_worker>> m_workers;
    std::uint32_t m_io_worker_id;

    static thread_local task_worker* m_current_worker;

    std::unique_ptr<task_block_queue> m_main_queue;
    std::unique_ptr<task_block_queue> m_global_queue;
    std::unique_ptr<task_block_queue> m_io_queue;
    std::vector<std::unique_ptr<task_work_queue>> m_work_queues;

    alignas(64) std::atomic<std::uint32_t> m_wake_sequence{0};
    alignas(64) std::atomic<std::uint32_t> m_io_wake_sequence{0};

    alignas(64) std::atomic<std::uint32_t> m_steal_cursor{0};

    static thread_local task_thread_type m_thread_type;

    std::vector<std::thread> m_threads;

    std::atomic<bool> m_running{false};

    std::unique_ptr<task_allocator_shared> m_shared_allocator;

    friend class task_handle;
    friend class task_event;
};

template <typename F>
task_handle task_handle::then(F&& fn, task_affinity affinity) const
{
    assert(is_valid());

    task_scheduler& scheduler = *m_block->scheduler;

    task_block* continuation = scheduler.allocate_block();
    continuation->reference_count.store(2, std::memory_order_relaxed);
    continuation->state.store(TASK_STATE_WAITING, std::memory_order_relaxed);
    continuation->dispatch_next = nullptr;
    continuation->affinity = affinity;
    continuation->function = std::forward<F>(fn);
    continuation->scheduler = m_block->scheduler;

    push_continuation(continuation);

    if (m_block->state.load(std::memory_order_acquire) == TASK_STATE_COMPLETE)
    {
        scheduler.dispatch_continuations(m_block);
    }

    task_handle handle;
    handle.m_block = continuation;

    return handle;
}

template <typename F>
task_handle task_event::then(F&& fn, task_affinity affinity)
{
    task_scheduler& scheduler = task_scheduler::instance();

    task_block* block = scheduler.allocate_block();
    block->reference_count.store(2, std::memory_order_relaxed);
    block->state.store(TASK_STATE_WAITING, std::memory_order_relaxed);
    block->dispatch_next = nullptr;
    block->affinity = affinity;
    block->function = std::forward<F>(fn);
    block->scheduler = &scheduler;

    bool ready;
    {
        std::lock_guard lock(m_mutex);

        if (m_fired.load(std::memory_order_acquire))
        {
            ready = true;
        }
        else if (m_count.load(std::memory_order_acquire) == 0)
        {
            m_fired.store(true, std::memory_order_release);
            ready = true;
        }
        else
        {
            m_waiters.push_back(block);
            ready = false;
        }
    }

    if (ready)
    {
        scheduler.enqueue(block);
    }

    task_handle handle;
    handle.m_block = block;
    return handle;
}
} // namespace violet