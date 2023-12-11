#include "vk_resource.hpp"
#include "vk_command.hpp"
#include "vk_image_loader.hpp"
#include "vk_util.hpp"
#include <cassert>

namespace violet::vk
{
namespace
{
std::uint32_t find_memory_type(
    std::uint32_t type_filter,
    VkMemoryPropertyFlags properties,
    VkPhysicalDevice physical_device)
{
    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);

    for (std::uint32_t i = 0; i < memory_properties.memoryTypeCount; ++i)
    {
        if ((type_filter & (1 << i)) &&
            (memory_properties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    throw vk_exception("Failed to find suitable memory type!");
}
} // namespace

vk_resource::vk_resource(vk_context* context) : m_context(context)
{
}

void vk_resource::create_device_local_buffer(
    const void* data,
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkBuffer& buffer,
    VkDeviceMemory& memory)
{
    assert(data != nullptr);

    VkBuffer staging_buffer;
    VkDeviceMemory staging_memory;
    create_host_visible_buffer(
        data,
        size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        staging_buffer,
        staging_memory);

    create_buffer(
        size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        buffer,
        memory);

    copy_buffer(staging_buffer, buffer, size);
    destroy_buffer(staging_buffer, staging_memory);
}

void vk_resource::create_host_visible_buffer(
    const void* data,
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkBuffer& buffer,
    VkDeviceMemory& memory)
{
    create_buffer(
        size,
        usage,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        buffer,
        memory);

    if (data != nullptr)
    {
        void* mapping_pointer = nullptr;
        vkMapMemory(m_context->get_device(), memory, 0, size, 0, &mapping_pointer);
        std::memcpy(mapping_pointer, data, size);
        vkUnmapMemory(m_context->get_device(), memory);
    }
}

void vk_resource::create_buffer(
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties,
    VkBuffer& buffer,
    VkDeviceMemory& memory)
{
    VkDevice device = m_context->get_device();

    VkBufferCreateInfo buffer_info = {};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size;
    buffer_info.usage = usage;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    vk_check(vkCreateBuffer(device, &buffer_info, nullptr, &buffer));

    VkMemoryRequirements requirements;
    vkGetBufferMemoryRequirements(device, buffer, &requirements);

    VkMemoryAllocateInfo allocate_info = {};
    allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.allocationSize = requirements.size;
    allocate_info.memoryTypeIndex =
        find_memory_type(requirements.memoryTypeBits, properties, m_context->get_physical_device());

    vk_check(vkAllocateMemory(device, &allocate_info, nullptr, &memory));
    vk_check(vkBindBufferMemory(device, buffer, memory, 0));
}

void vk_resource::destroy_buffer(VkBuffer buffer, VkDeviceMemory memory)
{
    VkDevice device = m_context->get_device();
    vkDestroyBuffer(device, buffer, nullptr);
    vkFreeMemory(device, memory, nullptr);
}

void vk_resource::copy_buffer(VkBuffer source, VkBuffer target, VkDeviceSize size)
{
    vk_graphics_queue* graphics_queue = m_context->get_graphics_queue();
    vk_command* command = graphics_queue->allocate_command();

    VkBufferCopy copy_region = {0, 0, size};
    vkCmdCopyBuffer(command->get_command_buffer(), source, target, 1, &copy_region);
    graphics_queue->execute_sync(command);
}

vk_image::vk_image(vk_context* context)
    : vk_resource(context),
      m_image(VK_NULL_HANDLE),
      m_memory(VK_NULL_HANDLE),
      m_image_view(VK_NULL_HANDLE),
      m_format(VK_FORMAT_UNDEFINED),
      m_extent{0, 0}
{
    m_clear_value.color = {0.0f, 0.0f, 0.0f, 1.0f};
}

vk_image::~vk_image()
{
    destroy_image(m_image, m_memory);
    destroy_image_view(m_image_view);
}

rhi_resource_format vk_image::get_format() const noexcept
{
    return vk_util::map_format(m_format);
}

void vk_image::create_image(
    const VkImageCreateInfo& image_info,
    VkMemoryPropertyFlags properties,
    VkImage& image,
    VkDeviceMemory& memory)
{
    auto device = get_context()->get_device();

    vk_check(vkCreateImage(device, &image_info, nullptr, &image));

    VkMemoryRequirements requirements;
    vkGetImageMemoryRequirements(device, image, &requirements);

    VkMemoryAllocateInfo allocate_info = {};
    allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.allocationSize = requirements.size;
    allocate_info.memoryTypeIndex = find_memory_type(
        requirements.memoryTypeBits,
        properties,
        get_context()->get_physical_device());

    vk_check(vkAllocateMemory(device, &allocate_info, nullptr, &memory));
    vk_check(vkBindImageMemory(device, image, memory, 0));
}

void vk_image::destroy_image(VkImage image, VkDeviceMemory memory)
{
    VkDevice device = get_context()->get_device();
    vkDestroyImage(device, image, nullptr);
    vkFreeMemory(device, memory, nullptr);
}

void vk_image::create_image_view(
    VkImage image,
    VkFormat format,
    VkImageViewType view_type,
    VkImageAspectFlags aspect_mask,
    VkImageView& image_view)
{
    VkImageViewCreateInfo image_view_info = {};
    image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_info.image = image;
    image_view_info.viewType = view_type;
    image_view_info.format = format;
    image_view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.subresourceRange.aspectMask = aspect_mask;
    image_view_info.subresourceRange.baseMipLevel = 0;
    image_view_info.subresourceRange.levelCount = 1;
    image_view_info.subresourceRange.baseArrayLayer = 0;
    image_view_info.subresourceRange.layerCount = view_type == VK_IMAGE_VIEW_TYPE_CUBE ? 6 : 1;
    vkCreateImageView(get_context()->get_device(), &image_view_info, nullptr, &image_view);
}

void vk_image::destroy_image_view(VkImageView image_view)
{
    vkDestroyImageView(get_context()->get_device(), image_view, nullptr);
}

void vk_image::copy_buffer_to_image(
    VkBuffer buffer,
    VkImage image,
    std::uint32_t width,
    std::uint32_t height)
{
    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};

    copy_buffer_to_image(buffer, image, {region});
}

void vk_image::copy_buffer_to_image(
    VkBuffer buffer,
    VkImage image,
    const std::vector<VkBufferImageCopy>& regions)
{
    vk_graphics_queue* graphics_queue = get_context()->get_graphics_queue();
    vk_command* command = graphics_queue->allocate_command();

    vkCmdCopyBufferToImage(
        command->get_command_buffer(),
        buffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        static_cast<std::uint32_t>(regions.size()),
        regions.data());

    graphics_queue->execute_sync(command);
}

void vk_image::transition_image_layout(
    VkImage image,
    VkImageLayout old_layout,
    VkImageLayout new_layout,
    std::uint32_t layer_count)
{
    vk_graphics_queue* graphics_queue = get_context()->get_graphics_queue();
    vk_command* command = graphics_queue->allocate_command();

    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = old_layout;
    barrier.newLayout = new_layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = layer_count;

    if (new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
        barrier.subresourceRange.aspectMask =
            VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    else
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    VkPipelineStageFlags source_stage;
    VkPipelineStageFlags destination_stage;

    if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
        new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (
        old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
        new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (
        old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
        new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destination_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    else if (
        old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
        new_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask =
            VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destination_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    }
    else
    {
        throw vk_exception("Unsupported layout transition!");
    }

    vkCmdPipelineBarrier(
        command->get_command_buffer(),
        source_stage,
        destination_stage,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &barrier);

    graphics_queue->execute_sync(command);
}

vk_depth_stencil::vk_depth_stencil(const rhi_depth_stencil_buffer_desc& desc, vk_context* context)
    : vk_image(context)
{
    VkImage image;
    VkDeviceMemory memory;

    VkFormat format = vk_util::map_format(desc.format);

    auto device = get_context()->get_device();

    VkImageCreateInfo image_info = {};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.extent.width = desc.width;
    image_info.extent.height = desc.height;
    image_info.extent.depth = 1;
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.format = format;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.samples = vk_util::map_sample_count(desc.samples);
    image_info.flags = 0;

    vk_check(vkCreateImage(device, &image_info, nullptr, &image));

    VkMemoryRequirements requirements;
    vkGetImageMemoryRequirements(device, image, &requirements);

    VkMemoryAllocateInfo allocate_info = {};
    allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.allocationSize = requirements.size;
    allocate_info.memoryTypeIndex = find_memory_type(
        requirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        get_context()->get_physical_device());

    vk_check(vkAllocateMemory(device, &allocate_info, nullptr, &memory));
    vk_check(vkBindImageMemory(device, image, memory, 0));

    VkImageView image_view;
    create_image_view(
        image,
        format,
        VK_IMAGE_VIEW_TYPE_2D,
        VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
        image_view);

    std::hash<void*> hasher;
    std::size_t hash = vk_util::hash_combine(hasher(image), hasher(image_view));

    set_image(image, memory);
    set_image_view(image_view);
    set_format(format);
    set_extent(VkExtent2D{desc.width, desc.height});
    set_clear_value(VkClearDepthStencilValue{1.0, 0});
    set_hash(hash);
}

vk_depth_stencil::~vk_depth_stencil()
{
}

vk_texture::vk_texture(const char* file, vk_context* context) : vk_image(context)
{
    vk_image_loader loader;
    if (!loader.load(file))
        throw vk_exception("Failed to load image.");

    const vk_image_data& data = loader.get_mipmap(0);

    VkBuffer staging_buffer;
    VkDeviceMemory staging_memory;
    create_host_visible_buffer(
        data.pixels.data(),
        data.pixels.size(),
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        staging_buffer,
        staging_memory);

    VkImageCreateInfo image_info = {};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.extent.width = data.width;
    image_info.extent.height = data.height;
    image_info.extent.depth = 1;
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.format = data.format;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.flags = 0;

    VkImage image;
    VkDeviceMemory memory;
    create_image(image_info, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, memory);

    transition_image_layout(image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copy_buffer_to_image(staging_buffer, image, data.width, data.height);
    transition_image_layout(
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    destroy_buffer(staging_buffer, staging_memory);

    VkImageView image_view;
    create_image_view(
        image,
        data.format,
        VK_IMAGE_VIEW_TYPE_2D,
        VK_IMAGE_ASPECT_COLOR_BIT,
        image_view);

    std::hash<void*> hasher;
    std::size_t hash = vk_util::hash_combine(hasher(image), hasher(image_view));

    set_image(image, memory);
    set_image_view(image_view);
    set_image_layout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    set_format(data.format);
    set_extent(VkExtent2D{data.width, data.height});
    set_hash(hash);
}

vk_texture::~vk_texture()
{
}

vk_texture_cube::vk_texture_cube(
    const char* right,
    const char* left,
    const char* top,
    const char* bottom,
    const char* front,
    const char* back,
    vk_context* context)
    : vk_image(context)
{
    std::vector<vk_image_loader> loaders(6);
    if (!loaders[0].load(right) || !loaders[1].load(left) || !loaders[2].load(top) ||
        !loaders[3].load(bottom) || !loaders[4].load(front) || !loaders[5].load(back))
        throw vk_exception("Failed to load image.");

    std::vector<std::size_t> data_offset(6);
    std::size_t data_size = 0;
    for (std::size_t i = 0; i < 6; ++i)
    {
        if (i > 0)
            data_offset[i] = data_offset[i - 1] + loaders[i - 1].get_mipmap(0).pixels.size();

        data_size += loaders[i].get_mipmap(0).pixels.size();
    }

    std::vector<char> data(data_size);
    for (std::size_t i = 0; i < 6; ++i)
    {
        std::memcpy(
            data.data() + data_offset[i],
            loaders[i].get_mipmap(0).pixels.data(),
            loaders[i].get_mipmap(0).pixels.size());
    }

    VkBuffer staging_buffer;
    VkDeviceMemory staging_memory;
    create_host_visible_buffer(
        data.data(),
        data.size(),
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        staging_buffer,
        staging_memory);

    VkImageCreateInfo image_info = {};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.extent.width = loaders[0].get_mipmap(0).width;
    image_info.extent.height = loaders[0].get_mipmap(0).height;
    image_info.extent.depth = 1;
    image_info.mipLevels = 1;
    image_info.arrayLayers = 6;
    image_info.format = loaders[0].get_mipmap(0).format;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

    VkImage image;
    VkDeviceMemory memory;
    create_image(image_info, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, memory);

    transition_image_layout(
        image,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        6);

    std::vector<VkBufferImageCopy> regions;
    for (std::uint32_t i = 0; i < 6; ++i)
    {
        VkBufferImageCopy region = {};
        region.bufferOffset = data_offset[i];
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = i;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {image_info.extent.width, image_info.extent.height, 1};

        regions.push_back(region);
    }

    copy_buffer_to_image(staging_buffer, image, regions);

    transition_image_layout(
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        6);

    destroy_buffer(staging_buffer, staging_memory);

    VkImageView image_view;
    create_image_view(
        image,
        image_info.format,
        VK_IMAGE_VIEW_TYPE_CUBE,
        VK_IMAGE_ASPECT_COLOR_BIT,
        image_view);

    std::hash<void*> hasher;
    std::size_t hash = vk_util::hash_combine(hasher(image), hasher(image_view));

    set_image(image, memory);
    set_image_view(image_view);
    set_image_layout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    set_format(image_info.format);
    set_extent(VkExtent2D{image_info.extent.width, image_info.extent.height});
    set_hash(hash);
}

vk_texture_cube::~vk_texture_cube()
{
}

vk_sampler::vk_sampler(const rhi_sampler_desc& desc, vk_context* context) : m_context(context)
{
    VkSamplerCreateInfo sampler_info = {};
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.magFilter = vk_util::map_filter(desc.mag_filter);
    sampler_info.minFilter = vk_util::map_filter(desc.min_filter);
    sampler_info.addressModeU = vk_util::map_sampler_address_mode(desc.address_mode_u);
    sampler_info.addressModeV = vk_util::map_sampler_address_mode(desc.address_mode_v);
    sampler_info.addressModeW = vk_util::map_sampler_address_mode(desc.address_mode_w);
    sampler_info.anisotropyEnable = VK_TRUE;
    sampler_info.maxAnisotropy =
        m_context->get_physical_device_properties().limits.maxSamplerAnisotropy;
    sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_info.unnormalizedCoordinates = VK_FALSE;
    sampler_info.compareEnable = VK_FALSE;
    sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    vk_check(vkCreateSampler(m_context->get_device(), &sampler_info, nullptr, &m_sampler));
}

vk_sampler::~vk_sampler()
{
    vkDestroySampler(m_context->get_device(), m_sampler, nullptr);
}

vk_buffer::vk_buffer(vk_context* context) : vk_resource(context)
{
}

vk_vertex_buffer::vk_vertex_buffer(const rhi_buffer_desc& desc, vk_context* context)
    : vk_buffer(context),
      m_mapping_pointer(nullptr)
{
    assert(desc.flags & RHI_BUFFER_FLAG_VERTEX);

    m_buffer_size = desc.size;

    VkBufferUsageFlags usage_flags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    if (desc.flags & RHI_BUFFER_FLAG_STORAGE)
        usage_flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

    if (desc.flags & RHI_BUFFER_FLAG_HOST_VISIBLE)
    {
        create_host_visible_buffer(desc.data, m_buffer_size, usage_flags, m_buffer, m_memory);
        vkMapMemory(get_context()->get_device(), m_memory, 0, m_buffer_size, 0, &m_mapping_pointer);
    }
    else
    {
        create_device_local_buffer(desc.data, m_buffer_size, usage_flags, m_buffer, m_memory);
    }
}

vk_vertex_buffer::~vk_vertex_buffer()
{
    if (m_mapping_pointer)
        vkUnmapMemory(get_context()->get_device(), m_memory);

    destroy_buffer(m_buffer, m_memory);
}

vk_index_buffer::vk_index_buffer(const rhi_buffer_desc& desc, vk_context* context)
    : vk_buffer(context),
      m_mapping_pointer(nullptr)
{
    assert(desc.flags & RHI_BUFFER_FLAG_INDEX);

    m_buffer_size = desc.size;

    VkBufferUsageFlags usage_flags = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    if (desc.flags & RHI_BUFFER_FLAG_STORAGE)
        usage_flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

    if (desc.index.size == 2)
        m_index_type = VK_INDEX_TYPE_UINT16;
    else if (desc.index.size == 4)
        m_index_type = VK_INDEX_TYPE_UINT32;
    else
        throw vk_exception("Invalid index size.");

    if (desc.flags & RHI_BUFFER_FLAG_HOST_VISIBLE)
    {
        create_host_visible_buffer(desc.data, m_buffer_size, usage_flags, m_buffer, m_memory);
        vkMapMemory(get_context()->get_device(), m_memory, 0, m_buffer_size, 0, &m_mapping_pointer);
    }
    else
    {
        create_device_local_buffer(desc.data, m_buffer_size, usage_flags, m_buffer, m_memory);
    }
}

vk_index_buffer::~vk_index_buffer()
{
    if (m_mapping_pointer)
        vkUnmapMemory(get_context()->get_device(), m_memory);

    destroy_buffer(m_buffer, m_memory);
}

vk_uniform_buffer::vk_uniform_buffer(void* data, std::size_t size, vk_context* context)
    : vk_buffer(context),
      m_mapping_pointer(nullptr)
{
    m_buffer_size = size;

    create_host_visible_buffer(
        data,
        m_buffer_size,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        m_buffer,
        m_memory);
    vkMapMemory(get_context()->get_device(), m_memory, 0, m_buffer_size, 0, &m_mapping_pointer);
}

vk_uniform_buffer::~vk_uniform_buffer()
{
    if (m_mapping_pointer)
        vkUnmapMemory(get_context()->get_device(), m_memory);

    destroy_buffer(m_buffer, m_memory);
}

vk_storage_buffer::vk_storage_buffer(const rhi_buffer_desc& desc, vk_context* context)
    : vk_buffer(context),
      m_mapping_pointer(nullptr)
{
    m_buffer_size = desc.size;

    if (desc.flags & RHI_BUFFER_FLAG_HOST_VISIBLE)
    {
        create_host_visible_buffer(
            desc.data,
            m_buffer_size,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            m_buffer,
            m_memory);
        vkMapMemory(get_context()->get_device(), m_memory, 0, m_buffer_size, 0, &m_mapping_pointer);
    }
    else
    {
        create_device_local_buffer(
            desc.data,
            m_buffer_size,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            m_buffer,
            m_memory);
    }
}

vk_storage_buffer::~vk_storage_buffer()
{
    if (m_mapping_pointer)
        vkUnmapMemory(get_context()->get_device(), m_memory);

    destroy_buffer(m_buffer, m_memory);
}
} // namespace violet::vk