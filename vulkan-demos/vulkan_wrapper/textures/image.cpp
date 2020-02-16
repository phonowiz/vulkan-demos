//
//  image.cpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 2/15/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#include "image.h"

using namespace vk;


VkImageCreateInfo image::get_image_create_info(VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage_flags)
{
    VkImageCreateInfo image_create_info = {};
    
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.pNext  = nullptr;
    image_create_info.flags = 0;
    image_create_info.imageType = _depth == 1 ? VK_IMAGE_TYPE_2D : VK_IMAGE_TYPE_3D;
    image_create_info.format = format;
    image_create_info.extent.width = _width;
    image_create_info.extent.height = _height;
    image_create_info.extent.depth = _depth;
    image_create_info.mipLevels = _mip_levels;
    image_create_info.arrayLayers = 1;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.tiling = tiling;
    image_create_info.usage = usage_flags;
    
    return image_create_info;
}

void image::create_image(  VkFormat format, VkImageTiling tiling,
                         VkImageUsageFlags usage_flags, VkMemoryPropertyFlags property_flags)
{
    
    VkImageCreateInfo image_create_info = get_image_create_info(format, tiling, usage_flags);
    
    //the following assignment depends on this assumption:
    assert(_device->_present_queue == _device->_graphics_queue);
    image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    uint32_t graphics_fam_index = _device->_queue_family_indices.graphics_family.value();
    image_create_info.queueFamilyIndexCount = 1;
    image_create_info.pQueueFamilyIndices = &graphics_fam_index;
    image_create_info.initialLayout = static_cast<VkImageLayout>(_image_layout);
    
    VkResult result = vkCreateImage(_device->_logical_device, &image_create_info, nullptr, &_image);
    ASSERT_VULKAN(result);
    VkMemoryRequirements memory_requirements {};
    vkGetImageMemoryRequirements(_device->_logical_device, _image, &memory_requirements);
    
    VkMemoryAllocateInfo memory_allocate_info;
    memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memory_allocate_info.pNext = nullptr;
    memory_allocate_info.allocationSize = memory_requirements.size;
    memory_allocate_info.memoryTypeIndex = find_memory_type_index(_device->_physical_device, memory_requirements.memoryTypeBits,
                                                                  property_flags);
    result = vkAllocateMemory(_device->_logical_device, &memory_allocate_info, nullptr, &_image_memory);
    
    ASSERT_VULKAN(result);
    
    result = vkBindImageMemory(_device->_logical_device, _image, _image_memory, 0);
    ASSERT_VULKAN(result);
}

void image::change_image_layout(VkCommandPool command_pool, VkQueue queue, VkImage image,
                                VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout)
{
    VkCommandBuffer command_buffer = _device->start_single_time_command_buffer( command_pool);
    
    VkImageMemoryBarrier image_memory_barrier {};
    image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    image_memory_barrier.pNext = nullptr;
    
    _image_layout = static_cast<image_layouts>(new_layout);
    VkPipelineStageFlags source_stage {};
    VkPipelineStageFlags destination_stage {};
    if( old_layout == VK_IMAGE_LAYOUT_PREINITIALIZED &&
       new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        image_memory_barrier.srcAccessMask = 0;
        image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        
    }
    else if( old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
            new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        image_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        
        source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if( old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
            new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        image_memory_barrier.srcAccessMask = 0;
        image_memory_barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        
        source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destination_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    else if( old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
            new_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
    {
        image_memory_barrier.srcAccessMask = 0;
        image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        
        source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if( old_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL &&
            new_layout == VK_IMAGE_LAYOUT_GENERAL)
    {
        image_memory_barrier.srcAccessMask = 0;
        image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        
        source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if( old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
            new_layout == VK_IMAGE_LAYOUT_GENERAL)
    {
        image_memory_barrier.srcAccessMask = 0;
        image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        
        source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else
    {
        assert(0 && "transition not yet supported");
    }
    image_memory_barrier.oldLayout = old_layout;
    image_memory_barrier.newLayout = new_layout;
    image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_memory_barrier.image = image;
    image_memory_barrier.subresourceRange.aspectMask = static_cast<VkImageAspectFlags>(_aspect_flag);
    
    image_memory_barrier.subresourceRange.baseMipLevel = 0;
    image_memory_barrier.subresourceRange.levelCount = _mip_levels;
    image_memory_barrier.subresourceRange.baseArrayLayer = 0;
    image_memory_barrier.subresourceRange.layerCount = _depth;
    
    vkCmdPipelineBarrier(command_buffer,
                         source_stage, //operations in this pipeline stage should occur before the barrier
                         destination_stage, //operations in this pipeline stage will wait on the barrier
                         0, 0, nullptr, 0, nullptr, 1, &image_memory_barrier);
    
    
    _device->end_single_time_command_buffer( queue, command_pool, command_buffer);
    
    
}
bool image::is_stencil_format(VkFormat format)
{
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void image::write_buffer_to_image(VkCommandPool commandPool, VkQueue queue, VkBuffer buffer)
{
    
    VkCommandBuffer command_buffer = _device->start_single_time_command_buffer( commandPool);
    
    
    VkBufferImageCopy buffer_image_copy;
    
    buffer_image_copy.bufferOffset = 0;
    buffer_image_copy.bufferRowLength = 0;
    buffer_image_copy.bufferImageHeight = 0;
    buffer_image_copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    buffer_image_copy.imageSubresource.mipLevel = 0;
    buffer_image_copy.imageSubresource.baseArrayLayer = 0;
    buffer_image_copy.imageSubresource.layerCount = _depth;
    buffer_image_copy.imageOffset = { 0, 0, 0};
    buffer_image_copy.imageExtent = { static_cast<uint32_t>(get_width()),
        static_cast<uint32_t>(get_height()),
        1};
    
    vkCmdCopyBufferToImage(command_buffer, buffer, _image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &buffer_image_copy);
    
    
    _device->end_single_time_command_buffer(queue, commandPool, command_buffer);
}

void image::destroy()
{
    vkDestroySampler(_device->_logical_device, _sampler, nullptr);
    vkDestroyImageView(_device->_logical_device, _image_view, nullptr);
    vkDestroyImage(_device->_logical_device, _image, nullptr);
    vkFreeMemory(_device->_logical_device, _image_memory, nullptr);
    _sampler = VK_NULL_HANDLE;
    _image_view = VK_NULL_HANDLE;
    _image = VK_NULL_HANDLE;
    _image_memory = VK_NULL_HANDLE;
}



