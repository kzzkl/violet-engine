#include "common/log.hpp"
#include "core/context/plugin.hpp"
#include "interface/graphics_interface.hpp"

namespace violet::sample
{
class vulkan_plugin : public plugin
{
public:
protected:
    virtual bool on_load() override
    {
        log::info("{} {}.{}", get_name(), get_version().major, get_version().minor);

        make_rhi make = static_cast<make_rhi>(find_symbol("make_rhi"));
        if (!make)
            return false;

        m_rhi = make();
        if (m_rhi == nullptr)
            return false;

        m_rhi->initialize({});

        return true;
    }

    virtual void on_unload() override
    {
        destroy_rhi destroy = static_cast<destroy_rhi>(find_symbol("destroy_rhi"));
        destroy(m_rhi);
        m_rhi = nullptr;
    }

private:
    rhi_interface* m_rhi;
};
} // namespace violet::sample

int main()
{
    violet::sample::vulkan_plugin plugin;
    plugin.load("violet-graphics-vulkan.dll");
    plugin.unload();

    return 0;
}