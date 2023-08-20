#pragma once

#include "core/plugin_interface.hpp"
#include <memory>
#include <string_view>

namespace violet
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

    inline std::string_view get_name() const noexcept { return m_name; }
    inline plugin_version get_version() const noexcept { return m_version; }

protected:
    void* find_symbol(std::string_view name);

    virtual bool on_load() { return true; }
    virtual void on_unload() {}

private:
    std::string m_name;
    plugin_version m_version;

    std::unique_ptr<dynamic_library> m_library;
    bool m_loaded;
};
} // namespace violet