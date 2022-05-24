#pragma once

#include "assert.hpp"
#include "index_generator.hpp"
#include <memory>
#include <string>
#include <vector>

namespace ash::core
{
struct event_index : public index_generator<event_index, std::size_t>
{
};

class dispatcher
{
public:
    virtual ~dispatcher() = default;
};

template <typename Functor>
class sequence_dispatcher : public dispatcher
{
public:
    void subscribe(std::string_view name, Functor&& functor)
    {
        m_process.push_back(std::pair<std::string, Functor>{name, std::forward<Functor>(functor)});
    }

    void unsubscribe(std::string_view name)
    {
        for (auto& process : m_process)
        {
            if (process.first == name)
            {
                std::swap(process, m_process.back());
                m_process.pop_back();
            }
        }
    }

    template <typename... Args>
    void publish(Args&&... args)
    {
        for (auto& process : m_process)
            process.second(std::forward<Args>(args)...);
    }

private:
    std::vector<std::pair<std::string, Functor>> m_process;
};

/*
struct sample_event
{
    using dispatcher = sequence_dispatcher<std::function<void()>>;
};
*/

template <typename T>
struct event_dispatcher
{
    using type = T::dispatcher;
};

template <typename T>
using event_dispatcher_t = event_dispatcher<T>::type;

class event
{
public:
    template <typename Event, typename... Args>
    void subscribe(std::string_view name, Args&&... args)
    {
        event_dispatcher<Event>()->subscribe(name, std::forward<Args>(args)...);
    }

    template <typename Event>
    void unsubscribe(std::string_view name)
    {
        event_dispatcher<Event>()->unsubscribe(name);
    }

    template <typename Event, typename... Args>
    void publish(Args&&... args)
    {
        event_dispatcher<Event>()->publish(std::forward<Args>(args)...);
    }

    template <typename Event, typename... Args>
    void register_event(Args&&... args)
    {
        std::size_t index = event_index::value<Event>();
        if (m_dispatchers.size() <= index)
            m_dispatchers.resize(index + 1);

        ASH_ASSERT(m_dispatchers[index] == nullptr);

        m_dispatchers[index] =
            std::make_unique<event_dispatcher_t<Event>>(std::forward<Args>(args)...);
    }

    template <typename Event>
    void unregister_event()
    {
        m_dispatchers[event_index::value<Event>()] = nullptr;
    }

private:
    template <typename Event>
    auto event_dispatcher()
    {
        using dispatcher_type = event_dispatcher_t<Event>;

        std::size_t index = event_index::value<Event>();
        return static_cast<dispatcher_type*>(m_dispatchers[index].get());
    };

    std::vector<std::unique_ptr<dispatcher>> m_dispatchers;
};
} // namespace ash::core