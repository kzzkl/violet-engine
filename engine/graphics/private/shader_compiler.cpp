#include "shader_compiler.hpp"
#include "common/log.hpp"
#include <vector>

namespace violet
{
using Microsoft::WRL::ComPtr;

shader_compiler::shader_compiler()
{
    check(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_compiler)));
    check(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(m_utils.ReleaseAndGetAddressOf())));
    check(m_utils->CreateDefaultIncludeHandler(m_include_handler.ReleaseAndGetAddressOf()));
}

std::vector<std::uint8_t> shader_compiler::compile(
    std::span<char> source,
    std::span<const wchar_t*> arguments)
{
    DxcBuffer source_buffer = {.Ptr = source.data(), .Size = source.size(), .Encoding = 0};

    ComPtr<IDxcResult> compile_result;
    check(m_compiler->Compile(
        &source_buffer,
        arguments.data(),
        static_cast<std::uint32_t>(arguments.size()),
        m_include_handler.Get(),
        IID_PPV_ARGS(&compile_result)));

    HRESULT hr;
    compile_result->GetStatus(&hr);

    ComPtr<IDxcBlobUtf8> error_msg;
    check(compile_result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&error_msg), nullptr));

    if (error_msg && error_msg->GetStringLength())
    {
        log::error("[graphics] shader compilation failed: {}", error_msg->GetStringPointer());
        check(hr);
    }

    ComPtr<IDxcBlob> shader_blob;
    check(compile_result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shader_blob), nullptr));

    std::vector<std::uint8_t> result(shader_blob->GetBufferSize());
    std::memcpy(result.data(), shader_blob->GetBufferPointer(), result.size());

    return result;
}
} // namespace violet