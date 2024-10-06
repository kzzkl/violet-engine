#pragma once

#include "common/hash.hpp"
#include "common/type_index.hpp"
#include "graphics/shader.hpp"
#include <format>
#include <memory>
#include <span>
#include <unordered_map>
#include <vector>

namespace violet
{
class rhi_deleter
{
public:
    rhi_deleter();
    rhi_deleter(rhi* rhi);

    void operator()(rhi_render_pass* render_pass);
    void operator()(rhi_shader* shader);
    void operator()(rhi_render_pipeline* render_pipeline);
    void operator()(rhi_compute_pipeline* compute_pipeline);
    void operator()(rhi_parameter* parameter);
    void operator()(rhi_framebuffer* framebuffer);
    void operator()(rhi_sampler* sampler);
    void operator()(rhi_buffer* buffer);
    void operator()(rhi_texture* texture);
    void operator()(rhi_swapchain* swapchain);
    void operator()(rhi_fence* fence);
    void operator()(rhi_semaphore* semaphore);

private:
    rhi* m_rhi;
};

template <typename T>
using rhi_ptr = std::unique_ptr<T, rhi_deleter>;

class shader_compiler;
class render_device
{
private:
    struct shader_index : public type_index<shader_index, std::size_t>
    {
    };

public:
    render_device();
    ~render_device();

    static render_device& instance();

    void initialize(rhi* rhi);
    void reset();

    rhi_command* allocate_command();
    void execute(
        std::span<rhi_command*> commands,
        std::span<rhi_semaphore*> signal_semaphores,
        std::span<rhi_semaphore*> wait_semaphores,
        rhi_fence* fence);

    void begin_frame();
    void end_frame();

    rhi_fence* get_in_flight_fence();

    std::size_t get_frame_count() const noexcept;
    std::size_t get_frame_resource_count() const noexcept;
    std::size_t get_frame_resource_index() const noexcept;

    template <typename T>
    rhi_shader* get_shader(std::span<std::wstring> defines = {})
    {
        if (m_shader_groups.size() <= shader_index::value<T>())
        {
            m_shader_groups.resize(shader_index::value<T>() + 1);
        }

        std::uint64_t hash = 0;
        for (auto& macro : defines)
        {
            hash ^= hash::city_hash_64(macro.data(), macro.size());
        }

        shader_group& group = m_shader_groups[shader_index::value<T>()];
        auto iter = group.variants.find(hash);
        if (iter == group.variants.end())
        {
            std::vector<const wchar_t*> arguments = {
                L"-I",
                L"assets/shaders/include",
                L"-Wno-ignored-attributes",
                L"-all-resources-bound",
#ifndef NDEBUG
                L"-Zi"
#endif
            };

            if (m_rhi->get_backend() == RHI_BACKEND_VULKAN)
            {
                arguments.push_back(L"-spirv");
#ifndef NDEBUG
                arguments.push_back(L"-fspv-extension=SPV_KHR_non_semantic_info");
                arguments.push_back(L"-fspv-debug=vulkan-with-source");
#endif
            }

            if constexpr (T::stage == RHI_SHADER_STAGE_VERTEX)
            {
                arguments.push_back(L"-T");
                arguments.push_back(L"vs_6_0");
                arguments.push_back(L"-E");
                arguments.push_back(L"vs_main");
            }
            else if constexpr (T::stage == RHI_SHADER_STAGE_FRAGMENT)
            {
                arguments.push_back(L"-T");
                arguments.push_back(L"ps_6_0");
                arguments.push_back(L"-E");
                arguments.push_back(L"fs_main");
            }
            else if constexpr (T::stage == RHI_SHADER_STAGE_COMPUTE)
            {
                arguments.push_back(L"-T");
                arguments.push_back(L"cs_6_0");
                arguments.push_back(L"-E");
                arguments.push_back(L"cs_main");
            }

            for (auto& macro : defines)
            {
                arguments.push_back(macro.data());
            }

            auto code = compile_shader(T::path, arguments);

            rhi_shader_desc desc = {};
            desc.code = code.data();
            desc.code_size = code.size();
            desc.stage = T::stage;

            if constexpr (has_inputs<T>)
            {
                desc.vertex.attributes = T::inputs.attributes;
                desc.vertex.attribute_count = T::inputs.attribute_count;
            }

            if constexpr (has_parameters<T>)
            {
                desc.parameters = T::parameters.parameters;
                desc.parameter_count = T::parameters.parameter_count;
            }

            m_shader_cache.emplace_back(m_rhi->create_shader(desc), m_rhi_deleter);
            rhi_shader* result = m_shader_cache.back().get();

            if constexpr (has_inputs<T>)
            {
                for (std::size_t i = 0; i < T::inputs.attribute_count; ++i)
                {
                    m_shader_info[result].vertex_attributes.push_back(T::inputs.attributes[i].name);
                }
            }

            group.variants[hash] = result;
            return result;
        }
        else
        {
            return iter->second;
        }
    }

    const std::vector<std::string>& get_vertex_attributes(rhi_shader* shader) const
    {
        return m_shader_info.at(shader).vertex_attributes;
    }

public:
    template <typename T>
    void set_name(T* object, std::string_view name) const
    {
#ifndef NDEBUG
        m_rhi->set_name(object, name.data());
#endif
    }

public:
    rhi_ptr<rhi_render_pass> create_render_pass(const rhi_render_pass_desc& desc);
    
    rhi_ptr<rhi_render_pipeline> create_pipeline(const rhi_render_pipeline_desc& desc);
    rhi_ptr<rhi_compute_pipeline> create_pipeline(const rhi_compute_pipeline_desc& desc);

    rhi_ptr<rhi_parameter> create_parameter(const rhi_parameter_desc& desc);
    rhi_ptr<rhi_framebuffer> create_framebuffer(const rhi_framebuffer_desc& desc);

    rhi_ptr<rhi_buffer> create_buffer(const rhi_buffer_desc& desc);
    rhi_ptr<rhi_sampler> create_sampler(const rhi_sampler_desc& desc);

    rhi_ptr<rhi_texture> create_texture(const rhi_texture_desc& desc);
    rhi_ptr<rhi_texture> create_texture_view(const rhi_texture_view_desc& desc);

    rhi_ptr<rhi_swapchain> create_swapchain(const rhi_swapchain_desc& desc);

    rhi_ptr<rhi_fence> create_fence(bool signaled);
    rhi_ptr<rhi_semaphore> create_semaphore();

    rhi_deleter& get_deleter() noexcept
    {
        return m_rhi_deleter;
    }

private:
    rhi* m_rhi{nullptr};
    rhi_deleter m_rhi_deleter;

    std::vector<std::uint8_t> compile_shader(
        std::string_view path,
        std::span<const wchar_t*> arguments);

    struct shader_info
    {
        std::vector<std::string> vertex_attributes;
    };

    struct shader_group
    {
        std::unordered_map<std::uint64_t, rhi_shader*> variants;
    };

    std::unordered_map<rhi_shader*, shader_info> m_shader_info;
    std::vector<shader_group> m_shader_groups;
    std::vector<rhi_ptr<rhi_shader>> m_shader_cache;

    std::unique_ptr<shader_compiler> m_shader_compiler;
};
} // namespace violet