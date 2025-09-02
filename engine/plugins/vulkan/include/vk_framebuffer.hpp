#pragma once

#include "algorithm/hash.hpp"
#include "vk_common.hpp"
#include <unordered_map>

namespace violet::vk
{
class vk_context;
class vk_framebuffer_manager
{
public:
    vk_framebuffer_manager(vk_context* context);
    ~vk_framebuffer_manager();

    VkFramebuffer allocate_framebuffer(
        VkRenderPass render_pass,
        const std::vector<VkImageView>& image_views,
        const VkExtent2D& extent);

    void notify_texture_deleted(VkImageView image_view);
    void notify_render_pass_deleted(VkRenderPass render_pass);

private:
    struct framebuffer_key
    {
        VkRenderPass render_pass;
        std::vector<VkImageView> image_views;

        bool operator==(const framebuffer_key& other) const noexcept
        {
            if (image_views.size() != other.image_views.size())
            {
                return false;
            }

            for (std::size_t i = 0; i < image_views.size(); ++i)
            {
                if (image_views[i] != other.image_views[i])
                {
                    return false;
                }
            }

            return render_pass == other.render_pass;
        }
    };

    struct framebuffer_hash
    {
        std::size_t operator()(const framebuffer_key& key) const noexcept
        {
            std::size_t hash = hash::city_hash_64(
                static_cast<const void*>(key.image_views.data()),
                key.image_views.size() * sizeof(VkImageView));

            hash = hash::combine(hash, std::hash<VkRenderPass>()(key.render_pass));

            return hash;
        }
    };

    std::unordered_map<framebuffer_key, VkFramebuffer, framebuffer_hash> m_framebuffers;

    vk_context* m_context;
};
} // namespace violet::vk