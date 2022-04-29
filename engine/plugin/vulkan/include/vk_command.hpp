#pragma once

#include "vk_common.hpp"
#include <vector>

namespace ash::graphics::vk
{
class vk_command : public render_command
{
public:
    vk_command(VkCommandBuffer command_buffer);

    virtual void begin(render_pass* pass, frame_buffer* frame_buffer) override;
    virtual void end(render_pass* pass) override;

    virtual void begin(render_pipeline* subpass) override;
    virtual void end(render_pipeline* subpass) override;

    virtual void parameter(std::size_t i, render_parameter*) override {}

    virtual void draw(
        resource* vertex,
        resource* index,
        std::size_t index_start,
        std::size_t index_end,
        std::size_t vertex_base) override
    {
    }

    void reset();
    VkCommandBuffer command_buffer() const noexcept { return m_command_buffer; }

private:
    VkCommandBuffer m_command_buffer;
};

class vk_command_queue
{
public:
    vk_command_queue(const VkQueue& queue, std::size_t frame_resource_count);
    ~vk_command_queue();

    vk_command* allocate_command();
    void execute(vk_command* command);
    void execute_batch() {}

    VkQueue queue() const noexcept { return m_queue; }

    void switch_frame_resources();

private:
    VkQueue m_queue;
    VkCommandPool m_command_pool;

    std::size_t m_command_counter;
    std::vector<std::vector<vk_command>> m_commands;
};
} // namespace ash::graphics::vk