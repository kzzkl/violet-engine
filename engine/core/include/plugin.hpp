#pragma once

#include "plugin_interface.hpp"
#include <memory>
#include <string_view>

namespace ash::core
{
class dynamic_library
{
public:
    virtual ~dynamic_library() = default;

    virtual bool load(std::string_view path) = 0;
    virtual void unload() = 0;

    virtual void* find_symbol(std::string_view name) = 0;
};

class plugin
{
public:
    plugin();
    virtual ~plugin();

    bool load(std::string_view path);
    void unload();

    inline std::string_view name() const noexcept { return m_name; }
    inline plugin_version version() const noexcept { return m_version; }

protected:
    void* find_symbol(std::string_view name);

    virtual bool do_load() { return true; }
    virtual void do_unload() {}

private:
    std::string m_name;
    plugin_version m_version;

    std::unique_ptr<dynamic_library> m_library;
};
} // namespace ash