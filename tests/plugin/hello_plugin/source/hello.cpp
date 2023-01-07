#include "plugin_interface.hpp"
#include <cstring>

extern "C"
{
    PLUGIN_API violet::core::external::plugin_info get_plugin_info()
    {
        using namespace violet::core::external;
        plugin_info info = {};

        char name[] = "hello-plugin";
        memcpy(info.name, name, sizeof(name));

        info.version.major = 1;
        info.version.minor = 0;

        return info;
    }

    PLUGIN_API int hello_add(int a, int b) { return a + b; }
}