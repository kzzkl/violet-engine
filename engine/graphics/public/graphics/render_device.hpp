#pragma once

#include "algorithm/hash.hpp"
#include "common/type_index.hpp"
#include "graphics/pipeline_state.hpp"
#include "graphics/shader.hpp"
#include <memory>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

namespace violet
{
using render_id = std::uint32_t;
static constexpr render_id INVALID_RENDER_ID = std::numeric_limits<render_id>::max();

class rhi_deleter
{
public:
    rhi_deleter();
    rhi_deleter(rhi* rhi);

    void operator()(rhi_render_pass* render_pass);
    void operator()(rhi_shader* shader);
    void operator()(rhi_raster_pipeline* raster_pipeline);
    void operator()(rhi_compute_pipeline* compute_pipeline);
    void operator()(rhi_parameter* parameter);
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

class raw_buffer;
class raw_texture;

template <typename T>
concept rhi_resource = std::is_same_v<T, rhi_buffer> || std::is_same_v<T, rhi_texture> ||
                       std::is_same_v<T, rhi_texture_srv> || std::is_same_v<T, rhi_texture_uav> ||
                       std::is_same_v<T, rhi_texture_rtv> || std::is_same_v<T, rhi_texture_dsv> ||
                       std::is_same_v<T, rhi_buffer_srv> || std::is_same_v<T, rhi_buffer_uav>;

class transient_allocator;
class render_device
{
public:
    render_device();
    ~render_device();

    static render_device& instance();

    void initialize(rhi* rhi);
    void reset();

    rhi_command* allocate_command();
    void execute(rhi_command* command, bool sync = false);

    void begin_frame();
    void end_frame();

    std::uint32_t get_frame_count() const noexcept;
    std::uint32_t get_frame_resource_count() const noexcept;
    std::uint32_t get_frame_resource_index() const noexcept;

    rhi_parameter* get_bindless_parameter() const noexcept;

    template <typename T>
    rhi_shader* get_shader(std::span<const std::wstring> defines = {})
    {
        shader_key key = {
            .index = shader_index::value<T>(),
        };
        key.defines.assign(defines.begin(), defines.end());

        auto iter = m_shaders.find(key);
        if (iter != m_shaders.end())
        {
            return iter->second.get();
        }

        auto code = compile_shader(T::path, T::entry_point, T::stage, defines);

        rhi_shader_desc desc = {
            .code = code.data(),
            .code_size = static_cast<std::uint32_t>(code.size()),
            .stage = T::stage,
            .entry_point = T::entry_point.data(),
        };

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

        if constexpr (has_constant<T>)
        {
            desc.push_constant_size = sizeof(T::constant_data);
        }

        auto shader = rhi_ptr<rhi_shader>(m_rhi->create_shader(desc), m_rhi_deleter);
        m_rhi->set_name(shader.get(), T::path.data());

        rhi_shader* result = shader.get();
        m_shaders[key] = std::move(shader);

        return result;
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

    rhi_ptr<rhi_raster_pipeline> create_pipeline(const rhi_raster_pipeline_desc& desc);
    rhi_ptr<rhi_compute_pipeline> create_pipeline(const rhi_compute_pipeline_desc& desc);

    rhi_ptr<rhi_parameter> create_parameter(const rhi_parameter_desc& desc);

    rhi_ptr<rhi_sampler> create_sampler(const rhi_sampler_desc& desc);
    rhi_ptr<rhi_buffer> create_buffer(const rhi_buffer_desc& desc);
    rhi_ptr<rhi_texture> create_texture(const rhi_texture_desc& desc);

    rhi_ptr<rhi_swapchain> create_swapchain(const rhi_swapchain_desc& desc);

    rhi_ptr<rhi_fence> create_fence();

    // Transient resources.
    rhi_parameter* allocate_parameter(const rhi_parameter_desc& desc);

    rhi_texture* allocate_texture(const rhi_texture_desc& desc);
    void free_texture(rhi_texture* texture);

    rhi_buffer* allocate_buffer(const rhi_buffer_desc& desc);
    void free_buffer(rhi_buffer* buffer);

    rhi_render_pass* get_render_pass(const rhi_render_pass_desc& desc);

    rhi_rasterizer_state* get_rasterizer_state(
        rhi_cull_mode cull_mode = RHI_CULL_MODE_NONE,
        rhi_polygon_mode polygon_mode = RHI_POLYGON_MODE_FILL)
    {
        return m_rasterizer_state.get_dynamic(cull_mode, polygon_mode);
    }

    template <
        rhi_cull_mode CullMode = RHI_CULL_MODE_NONE,
        rhi_polygon_mode PolygonMode = RHI_POLYGON_MODE_FILL>
    rhi_rasterizer_state* get_rasterizer_state()
    {
        return m_rasterizer_state.get_static<CullMode, PolygonMode>();
    }

    rhi_blend_state* get_blend_state(std::span<const rhi_attachment_blend> attachments = {})
    {
        return m_blend_state.get_dynamic(attachments);
    }

    template <typename... Attachments>
    rhi_blend_state* get_blend_state()
    {
        return m_blend_state.get_static<Attachments...>();
    }

    rhi_depth_stencil_state* get_depth_stencil_state(
        bool depth_enable = false,
        bool depth_write_enable = false,
        rhi_compare_op depth_compare_op = RHI_COMPARE_OP_NEVER,
        bool stencil_enable = false,
        rhi_stencil_state stencil_front = {},
        rhi_stencil_state stencil_back = {})
    {
        return m_depth_stencil_state.get_dynamic(
            depth_enable,
            depth_write_enable,
            depth_compare_op,
            stencil_enable,
            stencil_front,
            stencil_back);
    }

    template <
        bool DepthEnable = false,
        bool DepthWriteEnable = false,
        rhi_compare_op DepthCompareOp = RHI_COMPARE_OP_NEVER,
        bool StencilEnable = false,
        rhi_stencil_state StencilFront = {},
        rhi_stencil_state StencilBack = {}>
    rhi_depth_stencil_state* get_depth_stencil_state()
    {
        return m_depth_stencil_state.get_static<
            DepthEnable,
            DepthWriteEnable,
            DepthCompareOp,
            StencilEnable,
            StencilFront,
            StencilBack>();
    }

    rhi_raster_pipeline* get_pipeline(const rhi_raster_pipeline_desc& desc);
    rhi_compute_pipeline* get_pipeline(const rhi_compute_pipeline_desc& desc);

    rhi_sampler* get_sampler(const rhi_sampler_desc& desc);

    template <typename T>
    T* get_buildin_texture()
    {
        std::size_t index = buildin_texture_index::value<T>();
        if (m_buildin_textures.size() <= index)
        {
            m_buildin_textures.resize(index + 1);
        }

        if (m_buildin_textures[index] == nullptr)
        {
            m_buildin_textures[index] = std::make_unique<T>();
        }

        return static_cast<T*>(m_buildin_textures[index].get());
    }

private:
    struct shader_index : public type_index<shader_index, std::uint32_t, 0>
    {
    };

    struct shader_key
    {
        std::uint32_t index;
        std::vector<std::wstring> defines;

        bool operator==(const shader_key& other) const noexcept
        {
            return index == other.index && defines == other.defines;
        }
    };

    struct shader_hash
    {
        std::size_t operator()(const shader_key& key) const noexcept
        {
            std::uint64_t hash = std::hash<std::uint32_t>()(key.index);
            for (const auto& macro : key.defines)
            {
                hash ^= hash::xx_hash(macro.data(), macro.size());
            }

            return hash;
        }
    };

    struct buildin_texture_index : public type_index<buildin_texture_index, std::size_t, 1>
    {
    };

    void create_buildin_resources();

    std::vector<std::uint8_t> compile_shader(
        std::string_view path,
        std::string_view entry_point,
        rhi_shader_stage_flag stage,
        std::span<const std::wstring> defines);

    rhi* m_rhi{nullptr};
    rhi_deleter m_rhi_deleter;

    std::unordered_map<shader_key, rhi_ptr<rhi_shader>, shader_hash> m_shaders;

    std::unique_ptr<shader_compiler> m_shader_compiler;

    std::unique_ptr<material_manager> m_material_manager;
    std::unique_ptr<geometry_manager> m_geometry_manager;

    std::vector<std::unique_ptr<raw_texture>> m_buildin_textures;
    std::vector<rhi_ptr<rhi_sampler>> m_buildin_samplers;

    std::unique_ptr<transient_allocator> m_transient_allocator;

    rasterizer_state m_rasterizer_state;
    blend_state m_blend_state;
    depth_stencil_state m_depth_stencil_state;

    friend class rhi_deleter;
};
} // namespace violet