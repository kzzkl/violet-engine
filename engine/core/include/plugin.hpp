#pragma once

#include "core_exports.hpp"
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

class CORE_API plugin
{
public:
    using version = plugin_version;

public:
    plugin();
    virtual ~plugin();

    bool load(std::string_view path);
    void unload();

    std::string_view get_name() const { return m_name; }
    version get_version() const { return m_version; }

protected:
    void* find_symbol(std::string_view name);

    virtual bool do_load() { return true; }
    virtual void do_unload() {}

private:
    std::string m_name;
    version m_version;

    std::unique_ptr<dynamic_library> m_library;
};
} // namespace ash::core