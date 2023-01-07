#include "plugin.hpp"
#include "test_common.hpp"

using namespace violet::core;
using namespace violet::test;

TEST_CASE("Load plugin", "[plugin]")
{
    plugin p;

    CHECK(p.load("test-hello-plugin.dll") == true);

    CHECK(p.get_name() == "hello-plugin");
    CHECK(p.get_version().major == 1);
    CHECK(p.get_version().minor == 0);

    plugin p2;
    CHECK(p.load("") == false);
}

TEST_CASE("Find symbol", "[plugin]")
{
    hello_plugin hello;
    CHECK(hello.load("test-hello-plugin.dll") == true);

    int sum = hello.add(1, 2);
    CHECK(sum == 3);
}