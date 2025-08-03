#pragma once

#include "algorithm/hash.hpp"
#include "graphics/render_interface.hpp"
#include <mutex>
#include <span>
#include <unordered_map>

namespace violet
{
class rasterizer_state
{
public:
    rhi_rasterizer_state* get_dynamic(rhi_cull_mode cull_mode, rhi_polygon_mode polygon_mode)
    {
        std::scoped_lock lock(m_mutex);

        rhi_rasterizer_state key = {
            .cull_mode = cull_mode,
            .polygon_mode = polygon_mode,
        };

        auto iter = m_states.find(key);
        if (iter != m_states.end())
        {
            return iter->second.get();
        }

        m_states[key] = std::make_unique<rhi_rasterizer_state>(cull_mode, polygon_mode);
        return m_states[key].get();
    }

    template <rhi_cull_mode CullMode, rhi_polygon_mode PolygonMode>
    rhi_rasterizer_state* get_static()
    {
        static rhi_rasterizer_state* state = get_dynamic(CullMode, PolygonMode);
        return state;
    }

private:
    struct state_equal
    {
        bool operator()(const rhi_rasterizer_state& a, const rhi_rasterizer_state& b) const noexcept
        {
            return a.cull_mode == b.cull_mode && a.polygon_mode == b.polygon_mode;
        }
    };

    struct state_hash
    {
        std::size_t operator()(const rhi_rasterizer_state& value) const noexcept
        {
            return hash::city_hash_64(value);
        }
    };

    std::unordered_map<
        rhi_rasterizer_state,
        std::unique_ptr<rhi_rasterizer_state>,
        state_hash,
        state_equal>
        m_states;

    std::mutex m_mutex;
};

class blend_state
{
public:
    template <
        bool Enable = false,
        rhi_blend_factor SrcColorFactor = RHI_BLEND_FACTOR_ZERO,
        rhi_blend_factor DstColorFactor = RHI_BLEND_FACTOR_ZERO,
        rhi_blend_op ColorOp = RHI_BLEND_OP_ADD,
        rhi_blend_factor SrcAlphaFactor = RHI_BLEND_FACTOR_ZERO,
        rhi_blend_factor DstAlphaFactor = RHI_BLEND_FACTOR_ZERO,
        rhi_blend_op AlphaOp = RHI_BLEND_OP_ADD>
    struct attachment
    {
        static constexpr bool enable = Enable;
        static constexpr rhi_blend_factor src_color_factor = SrcColorFactor;
        static constexpr rhi_blend_factor dst_color_factor = DstColorFactor;
        static constexpr rhi_blend_op color_op = ColorOp;
        static constexpr rhi_blend_factor src_alpha_factor = SrcAlphaFactor;
        static constexpr rhi_blend_factor dst_alpha_factor = DstAlphaFactor;
    };

    rhi_blend_state* get_dynamic(std::span<const rhi_attachment_blend> attachments = {})
    {
        rhi_blend_state state = {};
        for (std::size_t i = 0; i < attachments.size(); ++i)
        {
            state.attachments[i] = attachments[i];
        }

        std::scoped_lock lock(m_mutex);

        auto iter = m_states.find(state);
        if (iter != m_states.end())
        {
            return iter->second.get();
        }

        m_states[state] = std::make_unique<rhi_blend_state>(state);
        return m_states[state].get();
    }

    template <typename... Attachments>
    rhi_blend_state* get_static()
    {
        static rhi_blend_state* state = get_rhi<Attachments...>();
        return state;
    }

private:
    template <typename... Attachments>
    rhi_blend_state* get_rhi()
    {
        std::vector<rhi_attachment_blend> attachments(sizeof...(Attachments));
        std::size_t i = 0;
        ((attachments[i++] =
              {
                  Attachments::enable,
                  Attachments::src_color_factor,
                  Attachments::dst_color_factor,
                  Attachments::color_op,
                  Attachments::src_alpha_factor,
                  Attachments::dst_alpha_factor,
              }),
         ...);

        return get_dynamic(attachments);
    }

    struct state_equal
    {
        bool operator()(const rhi_blend_state& a, const rhi_blend_state& b) const noexcept
        {
            for (std::size_t i = 0; i < rhi_constants::max_attachments; ++i)
            {
                if (a.attachments[i].enable != b.attachments[i].enable ||
                    a.attachments[i].src_color_factor != b.attachments[i].src_color_factor ||
                    a.attachments[i].dst_color_factor != b.attachments[i].dst_color_factor ||
                    a.attachments[i].color_op != b.attachments[i].color_op ||
                    a.attachments[i].src_alpha_factor != b.attachments[i].src_alpha_factor ||
                    a.attachments[i].dst_alpha_factor != b.attachments[i].dst_alpha_factor ||
                    a.attachments[i].alpha_op != b.attachments[i].alpha_op)
                {
                    return false;
                }
            }

            return true;
        }
    };

    struct state_hash
    {
        std::size_t operator()(const rhi_blend_state& value) const noexcept
        {
            return hash::city_hash_64(value);
        }
    };

    std::unordered_map<rhi_blend_state, std::unique_ptr<rhi_blend_state>, state_hash, state_equal>
        m_states;

    std::mutex m_mutex;
};

class depth_stencil_state
{
public:
    rhi_depth_stencil_state* get_dynamic(
        bool depth_enable = false,
        bool depth_write_enable = false,
        rhi_compare_op depth_compare_op = RHI_COMPARE_OP_NEVER,
        bool stencil_enable = false,
        rhi_stencil_state stencil_front = {},
        rhi_stencil_state stencil_back = {})
    {
        std::scoped_lock lock(m_mutex);

        rhi_depth_stencil_state state = {
            .depth_enable = depth_enable,
            .depth_write_enable = depth_write_enable,
            .depth_compare_op = depth_compare_op,
            .stencil_enable = stencil_enable,
            .stencil_front = stencil_front,
            .stencil_back = stencil_back,
        };

        auto iter = m_states.find(state);
        if (iter != m_states.end())
        {
            return iter->second.get();
        }

        m_states[state] = std::make_unique<rhi_depth_stencil_state>(state);
        return m_states[state].get();
    }

    template <
        bool DepthEnable = false,
        bool DepthWriteEnable = false,
        rhi_compare_op DepthCompareOp = RHI_COMPARE_OP_NEVER,
        bool StencilEnable = false,
        rhi_stencil_state StencilFront = {},
        rhi_stencil_state StencilBack = {}>
    rhi_depth_stencil_state* get_static()
    {
        static rhi_depth_stencil_state* state = get_dynamic(
            DepthEnable,
            DepthWriteEnable,
            DepthCompareOp,
            StencilEnable,
            StencilFront,
            StencilBack);
        return state;
    }

private:
    struct state_equal
    {
        bool operator()(const rhi_depth_stencil_state& a, const rhi_depth_stencil_state& b)
            const noexcept
        {
            return a.depth_enable == b.depth_enable &&
                   a.depth_write_enable == b.depth_write_enable &&
                   a.depth_compare_op == b.depth_compare_op &&
                   a.stencil_enable == b.stencil_enable &&
                   a.stencil_front.compare_op == b.stencil_front.compare_op &&
                   a.stencil_front.pass_op == b.stencil_front.pass_op &&
                   a.stencil_front.fail_op == b.stencil_front.fail_op &&
                   a.stencil_front.depth_fail_op == b.stencil_front.depth_fail_op &&
                   a.stencil_front.reference == b.stencil_front.reference &&
                   a.stencil_back.compare_op == b.stencil_back.compare_op &&
                   a.stencil_back.pass_op == b.stencil_back.pass_op &&
                   a.stencil_back.fail_op == b.stencil_back.fail_op &&
                   a.stencil_back.depth_fail_op == b.stencil_back.depth_fail_op &&
                   a.stencil_back.reference == b.stencil_back.reference;
        }
    };

    struct state_hash
    {
        std::size_t operator()(const rhi_depth_stencil_state& value) const noexcept
        {
            return hash::city_hash_64(value);
        }
    };

    std::unordered_map<
        rhi_depth_stencil_state,
        std::unique_ptr<rhi_depth_stencil_state>,
        state_hash,
        state_equal>
        m_states;

    std::mutex m_mutex;
};
} // namespace violet