#pragma once

#include "plugin.hpp"
#include <catch2/catch.hpp>

namespace violet::test
{
class hello_plugin : public violet::plugin
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
} // namespace violet::test