#pragma once

#include "common/hash.hpp"
#include "graphics/shader.hpp"
#include <memory>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

namespace violet
{
using render_id = std::uint64_t;
static constexpr render_id INVALID_RENDER_ID = std::numeric_limits<render_id>::max();

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

private:
    rhi* m_rhi;
};

template <typename T>
using rhi_ptr = std::unique_ptr<T, rhi_deleter>;

class material_manager;
class geometry_manager;
class shader_compiler;

class render_device
{
public:
    render_device();
    ~render_device();

    static render_device& instance();

    void initialize(rhi* rhi);
    void reset();

    rhi_command* allocate_command();
    void execute(rhi_command* command);
    void execute_sync(rhi_command* command);

    void begin_frame();
    void end_frame();

    void fill_scene_data(shader::scene_data& scene);

    std::size_t get_frame_count() const noexcept;
    std::size_t get_frame_resource_count() const noexcept;
    std::size_t get_frame_resource_index() const noexcept;

    rhi_parameter* get_bindless_parameter() const noexcept;

    template <typename T>
    rhi_shader* get_shader(std::span<std::wstring> defines = {})
    {
        std::uint64_t hash =
            hash::combine(hash::city_hash_64(T::path.data(), T::path.size()), T::stage);
        for (auto& macro : defines)
        {
            hash ^= hash::city_hash_64(macro.data(), macro.size());
        }

        auto iter = m_shaders.find(hash);
        if (iter != m_shaders.end())
        {
            return iter->second.get();
        }

        std::vector<const wchar_t*> arguments = {
            L"-I",
            L"assets/shaders",
            L"-Wno-ignored-attributes",
            L"-all-resources-bound",
#ifndef NDEBUG
            L"-Zi",
            L"-Qembed_debug",
            L"-O0",
#endif
        };

        if (m_rhi->get_backend() == RHI_BACKEND_VULKAN)
        {
            arguments.push_back(L"-spirv");
            arguments.push_back(L"-fspv-target-env=vulkan1.3");
            arguments.push_back(L"-fvk-use-dx-layout");
            arguments.push_back(L"-fspv-extension=SPV_EXT_descriptor_indexing");
            arguments.push_back(L"-fvk-bind-resource-heap");
            arguments.push_back(L"0");
            arguments.push_back(L"0");
            arguments.push_back(L"-fvk-bind-sampler-heap");
            arguments.push_back(L"1");
            arguments.push_back(L"0");
#ifndef NDEBUG
            // arguments.push_back(L"-fspv-extension=SPV_KHR_non_semantic_info");
            // arguments.push_back(L"-fspv-debug=vulkan-with-source");
#endif
        }

        if constexpr (T::stage == RHI_SHADER_STAGE_VERTEX)
        {
            arguments.push_back(L"-T");
            arguments.push_back(L"vs_6_6");
            arguments.push_back(L"-E");
            arguments.push_back(L"vs_main");
        }
        else if constexpr (T::stage == RHI_SHADER_STAGE_FRAGMENT)
        {
            arguments.push_back(L"-T");
            arguments.push_back(L"ps_6_6");
            arguments.push_back(L"-E");
            arguments.push_back(L"fs_main");
        }
        else if constexpr (T::stage == RHI_SHADER_STAGE_COMPUTE)
        {
            arguments.push_back(L"-T");
            arguments.push_back(L"cs_6_6");
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

        auto shader = rhi_ptr<rhi_shader>(m_rhi->create_shader(desc), m_rhi_deleter);

        if constexpr (has_inputs<T>)
        {
            auto& vertex_attributes = m_vertex_attributes[shader.get()];
            for (std::size_t i = 0; i < T::inputs.attribute_count; ++i)
            {
                vertex_attributes.push_back(T::inputs.attributes[i].name);
            }
        }

        rhi_shader* result = shader.get();
        m_shaders[hash] = std::move(shader);

        return result;
    }

    const std::vector<std::string>& get_vertex_attributes(rhi_shader* shader) const
    {
        return m_vertex_attributes.at(shader);
    }

    material_manager* get_material_manager() const noexcept
    {
        return m_material_manager.get();
    }

    geometry_manager* get_geometry_manager() const noexcept
    {
        return m_geometry_manager.get();
    }

    template <typename T>
    void set_name(T* object, std::string_view name) const
    {
#ifndef NDEBUG
        m_rhi->set_name(object, name.data());
#endif
    }

    rhi_ptr<rhi_render_pass> create_render_pass(const rhi_render_pass_desc& desc);

    rhi_ptr<rhi_render_pipeline> create_pipeline(const rhi_render_pipeline_desc& desc);
    rhi_ptr<rhi_compute_pipeline> create_pipeline(const rhi_compute_pipeline_desc& desc);

    rhi_ptr<rhi_parameter> create_parameter(const rhi_parameter_desc& desc);
    rhi_ptr<rhi_framebuffer> create_framebuffer(const rhi_framebuffer_desc& desc);

    rhi_ptr<rhi_buffer> create_buffer(const rhi_buffer_desc& desc);
    rhi_ptr<rhi_sampler> create_sampler(const rhi_sampler_desc& desc);

    rhi_ptr<rhi_texture> create_texture(const rhi_texture_desc& desc);
    rhi_ptr<rhi_texture> create_texture(const rhi_texture_view_desc& desc);

    rhi_ptr<rhi_swapchain> create_swapchain(const rhi_swapchain_desc& desc);

    rhi_ptr<rhi_fence> create_fence();

    rhi_deleter& get_deleter() noexcept
    {
        return m_rhi_deleter;
    }

private:
    void create_buildin_resources();
    void create_buildin_samplers();

    std::vector<std::uint8_t> compile_shader(
        std::string_view path,
        std::span<const wchar_t*> arguments);

    rhi* m_rhi{nullptr};
    rhi_deleter m_rhi_deleter;

    std::unordered_map<std::uint64_t, rhi_ptr<rhi_shader>> m_shaders;
    std::unordered_map<rhi_shader*, std::vector<std::string>> m_vertex_attributes;

    std::unique_ptr<shader_compiler> m_shader_compiler;

    rhi_ptr<rhi_fence> m_fence;
    std::uint64_t m_fence_value{0};

    std::unique_ptr<material_manager> m_material_manager;
    std::unique_ptr<geometry_manager> m_geometry_manager;

    rhi_ptr<rhi_texture> m_empty_texture;
    rhi_ptr<rhi_texture> m_brdf_lut;

    rhi_ptr<rhi_sampler> m_point_repeat_sampler;
    rhi_ptr<rhi_sampler> m_point_clamp_sampler;
    rhi_ptr<rhi_sampler> m_linear_repeat_sampler;
    rhi_ptr<rhi_sampler> m_linear_clamp_sampler;
};
} // namespace violet