#pragma once

#include <cstddef>

namespace violet
{
template <typename... Types>
class type_list
{
public:
    template <typename... AddTypes>
    using add = type_list<Types..., AddTypes...>;

private:
    template <typename... Types>
    struct template_index;

    template <typename T, typename... Types>
    struct template_index<T, T, Types...>
    {
        static constexpr std::size_t value = 0;
    };

    template <typename T, typename U, typename... Types>
    struct template_index<T, U, Types...>
    {
        static_assert(sizeof...(Types) != 0);
        static constexpr std::size_t value = 1 + template_index<T, Types...>::value;
    };

    template <typename T, typename... Types>
    static constexpr std::size_t template_index_v = template_index<T, Types...>::value;

public:
    template <typename Functor>
    static void each(Functor&& f)
    {
        (f.template operator()<Types>(), ...);
    }

    template <typename T>
    static constexpr std::size_t index()
    {
        return template_index_v<T, Types...>;
    }

    template <typename T>
    constexpr static bool has()
    {
        return (std::is_same<T, Types>::value || ...);
    }
};
} // namespace violet