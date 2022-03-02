#pragma once

#include "plugin.hpp"
#include <catch2/catch.hpp>

namespace ash::test
{
class hello_plugin : public ash::core::plugin
{
public:
    hello_plugin();

    int add(int a, int b);

protected:
    virtual bool do_load() override;
    virtual void do_unload() override;

private:
    using add_impl = int (*)(int, int);
    add_impl m_add_impl;
};
} // namespace ash::test