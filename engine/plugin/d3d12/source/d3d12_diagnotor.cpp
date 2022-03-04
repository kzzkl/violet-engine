#include "d3d12_diagnotor.hpp"
#include "d3d12_context.hpp"

using namespace Microsoft::WRL;
using namespace ash::graphics::external;

namespace ash::graphics::d3d12
{
void d3d12_diagnotor::initialize()
{
    auto factory = d3d12_context::instance().get_factory();

    ComPtr<DXGIAdapter> adapter;

    for (UINT i = 0;; ++i)
    {
        if (factory->EnumAdapters1(i, adapter.GetAddressOf()) == DXGI_ERROR_NOT_FOUND)
            break;

        d3d12_adapter_info info;

        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);

        char description_buffer[128] = {};
        WideCharToMultiByte(
            CP_UTF8,
            0,
            desc.Description,
            static_cast<int>(wcslen(desc.Description)),
            description_buffer,
            static_cast<int>(sizeof(description_buffer)),
            nullptr,
            nullptr);

        info.description = description_buffer;

        m_adapter_info.push_back(info);
    }
}

int d3d12_diagnotor::get_adapter_info(adapter_info* infos, int size)
{
    int i = 0;
    for (; i < size && i < m_adapter_info.size(); ++i)
    {
        memcpy(
            infos[i].description,
            m_adapter_info[i].description.c_str(),
            m_adapter_info[i].description.size());
    }

    return i;
}
} // namespace ash::graphics::d3d12