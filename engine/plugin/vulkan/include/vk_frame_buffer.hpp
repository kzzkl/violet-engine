#pragma once

#include "vk_common.hpp"
#include "vk_resource.hpp"

namespace ash::graphics::vk
{
class vk_frame_buffer_layout
{
public:
    struct attachment_info
    {
        attachment_type type;
        VkAttachmentDescription description;
    };

public:
    vk_frame_buffer_layout(attachment_desc* attachment, std::size_t count);

    auto begin() noexcept { return m_attachments.begin(); }
    auto end() noexcept { return m_attachments.end(); }

    auto begin() const noexcept { return m_attachments.begin(); }
    auto end() const noexcept { return m_attachments.end(); }

    auto cbegin() const noexcept { return m_attachments.cbegin(); }
    auto cend() const noexcept { return m_attachments.cend(); }

private:
    std::vector<attachment_info> m_attachments;
};

class vk_render_pass;
class vk_frame_buffer
{
public:
    vk_frame_buffer(vk_render_pass* render_pass, vk_image* render_target);
    vk_frame_buffer(vk_frame_buffer&& other);
    ~vk_frame_buffer();

    VkFramebuffer frame_buffer() const noexcept { return m_frame_buffer; }
    const std::vector<VkClearValue>& clear_values() const noexcept { return m_clear_values; }

    vk_frame_buffer& operator=(vk_frame_buffer&& other);

private:
    VkFramebuffer m_frame_buffer;

    std::vector<std::unique_ptr<vk_image>> m_attachments;
    std::vector<VkClearValue> m_clear_values;
};

class vk_frame_buffer_manager
{
public:
    vk_frame_buffer* get_or_create_frame_buffer(
        vk_render_pass* render_pass,
        vk_image* render_target);

    void notify_destroy(vk_image* render_target);

private:
    using key_type = std::pair<vk_render_pass*, VkImageView>;
    struct key_hash
    {
        template <class T1, class T2>
        std::size_t operator()(const std::pair<T1, T2>& pair) const
        {
            return std::hash<T1>()(pair.first) ^ std::hash<T2>()(pair.second);
        }
    };

    std::unordered_map<key_type, std::unique_ptr<vk_frame_buffer>, key_hash> m_frame_buffers;
};
} // namespace ash::graphics::vk