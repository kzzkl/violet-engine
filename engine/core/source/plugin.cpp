#include "core/plugin.hpp"
#include "log.hpp"
#include <Windows.h>

namespace violet::core
{
class dynamic_library_win32 : public dynamic_library
{
public:
    dynamic_library_win32();
    virtual ~dynamic_library_win32();

    virtual bool load(std::string_view path) override;
    virtual void unload() override;

    virtual void* find_symbol(std::string_view name) override;

private:
    HINSTANCE m_lib;
};

dynamic_library_win32::dynamic_library_win32() : m_lib(nullptr)
{
}

dynamic_library_win32::~dynamic_library_win32()
{
    if (m_lib)
        unload();
}

bool dynamic_library_win32::load(std::string_view path)
{
    m_lib = LoadLibraryExA(path.data(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (m_lib)
    {
        log::debug("The dynamic library was loaded successfully: {}", path);
        return true;
    }
    else
    {
        log::error("Failed to load dynamic lib: paht[{}] error[{}]", path, GetLastError());
        return false;
    }
}

void dynamic_library_win32::unload()
{
    FreeLibrary(m_lib);
}

void* dynamic_library_win32::find_symbol(std::string_view name)
{
    return GetProcAddress(m_lib, name.data());
}

plugin::plugin()
    : m_name("null"),
      m_library(std::make_unique<dynamic_library_win32>()),
      m_loaded(false)
{
}

plugin::~plugin()
{
}

bool plugin::load(std::string_view path)
{
    if (m_loaded)
    {
        log::error("The plugin has been loaded before.");
        return false;
    }

    if (!m_library->load(path))
    {
        log::error("Failed to load plugin: name[{}] path[{}]", m_name, path);
        return false;
    }

    get_plugin_info get_info =
        static_cast<get_plugin_info>(m_library->find_symbol("get_plugin_info"));
    if (!get_info)
    {
        log::error("Symbol not found in dynamic library: get_plugin_info");
        m_library->unload();
        return false;
    }
    else
    {
        plugin_info info = get_info();
        m_name = info.name;
        m_version = info.version;
    }

    if (on_load() == false)
    {
        m_library->unload();
        return false;
    }

    m_loaded = true;
    return true;
}

void plugin::unload()
{
    if (m_loaded)
    {
        on_unload();
        m_library->unload();
    }
}

void* plugin::find_symbol(std::string_view name)
{
    return m_library->find_symbol(name);
}
} // namespace violet::core