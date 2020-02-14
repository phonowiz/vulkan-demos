//
//  render_texture.cpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 4/17/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#include "render_texture.h"
#include "device.h"
#include "texture_2d.h"

using namespace vk;


render_texture::render_texture(device* device, uint32_t width, uint32_t height):
texture_2d(device, width, height)
{
}


void render_texture::create(uint32_t width, uint32_t height)
{
    create_image(
                 static_cast<VkFormat>(_format),
                 VK_IMAGE_TILING_OPTIMAL,
                 static_cast<VkImageUsageFlagBits>(_usage),
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    

    create_image_view(_image, static_cast<VkFormat>(_format), _image_view);
    
    VkCommandBuffer command_buffer =
        _device->start_single_time_command_buffer(_device->_graphics_command_pool);
    
    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = _image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.oldLayout = static_cast<VkImageLayout>(_image_layout);
    barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.subresourceRange.layerCount = _depth;
    barrier.subresourceRange.levelCount = 1;
    
    vkCmdPipelineBarrier(command_buffer,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                         0, nullptr,
                         0, nullptr,
                         1, &barrier);
    
    _device->end_single_time_command_buffer(_device->_graphics_queue, _device->_graphics_command_pool, command_buffer);
    
}

