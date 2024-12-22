#include "core/plugin.hpp"
#include "common/log.hpp"
#include <Windows.h>

namespace violet
{
class dynamic_library_win32 : public dynamic_library
{
public:
    dynamic_library_win32() = default;
    virtual ~dynamic_library_win32();

    bool load(std::string_view path) override;
    void unload() override;

    void* find_symbol(std::string_view name) override;

private:
    HINSTANCE m_lib{};
};

dynamic_library_win32::~dynamic_library_win32()
{
    if (m_lib)
    {
        unload();
    }
}

bool dynamic_library_win32::load(std::string_view path)
{
    m_lib = LoadLibraryExA(path.data(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (!m_lib)
    {
        log::error("Failed to load dynamic lib: path[{}] error[{}]", path, GetLastError());
        return false;
    }

    log::debug("The dynamic library was loaded successfully: {}", path);

    return true;
}

void dynamic_library_win32::unload()
{
    FreeLibrary(m_lib);
}

void* dynamic_library_win32::find_symbol(std::string_view name)
{
    return reinterpret_cast<void*>(GetProcAddress(m_lib, name.data()));
}

plugin::plugin()
    : m_name("null"),
      m_library(std::make_unique<dynamic_library_win32>()),
      m_loaded(false)
{
}

plugin::~plugin() {}

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

    auto get_info = reinterpret_cast<get_plugin_info>(m_library->find_symbol("get_plugin_info"));
    if (!get_info)
    {
        log::error("Symbol not found in dynamic library: get_plugin_info");
        m_library->unload();
        return false;
    }

    plugin_info info = get_info();
    m_name = info.name;
    m_version = info.version;

    if (!on_load())
    {
        log::error("Plugin load failed");
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
} // namespace violet