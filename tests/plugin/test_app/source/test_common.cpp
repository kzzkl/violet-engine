#include "test_common.hpp"

namespace violet::test
{
hello_plugin::hello_plugin() : m_add_impl(nullptr)
{
}

int hello_plugin::add(int a, int b)
{
    if (m_add_impl)
        return m_add_impl(a, b);
    else
        return 0;
}

bool hello_plugin::do_load()
{
    m_add_impl = static_cast<add_impl>(find_symbol("hello_add"));
    return m_add_impl != nullptr;
}

void hello_plugin::do_unload()
{
}
} // namespace violet::test