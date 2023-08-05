#pragma once

#include <functional>
#include <tuple>

namespace violet
{
template <typename T>
struct function_traits : public function_traits<decltype(&T::operator())>
{
};

template <typename R, typename... Args>
struct function_traits<R(Args...)>
{
    using argument_type = std::tuple<Args...>;
    using return_type = R;
    using function_type = std::function<R(Args...)>;
};

template <typename R, typename... Args>
struct function_traits<R (*)(Args...)>
{
    using argument_type = std::tuple<Args...>;
    using return_type = R;
    using function_type = std::function<R(Args...)>;
};

template <typename R, typename C, typename... Args>
struct function_traits<R (C::*)(Args...) const>
{
    using argument_type = std::tuple<Args...>;
    using return_type = R;
    using function_type = std::function<R(Args...)>;
};

template <typename R, typename C, typename... Args>
struct function_traits<R (C::*)(Args...)>
{
    using argument_type = std::tuple<Args...>;
    using return_type = R;
    using function_type = std::function<R(Args...)>;
};
} // namespace violet