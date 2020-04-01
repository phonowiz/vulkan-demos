//
//  texture_3d.cpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 7/18/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#include "texture_3d.h"
#include <cmath>

using namespace vk;


void texture_3d::init()
{
    
    create_sampler();
    
    create_image(
                 static_cast<VkFormat>(_format),
                 VK_IMAGE_TILING_OPTIMAL,
                 VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
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
    barrier.newLayout = static_cast<VkImageLayout>(image_layouts::GENERAL);
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

    //texture_3d's don't have any layers, but texture_2d_arrays do...
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    vkCmdPipelineBarrier(command_buffer,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                         0, nullptr,
                         0, nullptr,
                         1, &barrier);

    _image_layout = image_layouts::GENERAL;

    _device->
        end_single_time_command_buffer(_device->_graphics_queue, _device->_graphics_command_pool, command_buffer);
    
    _initialized = true;
}

void texture_3d::create_sampler()
{
    VkSamplerCreateInfo sampler_create_info {};
    
    sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_create_info.pNext = nullptr;
    sampler_create_info.flags = 0;
    sampler_create_info.magFilter = static_cast<VkFilter>(_filter);
    sampler_create_info.minFilter = static_cast<VkFilter>(_filter);
    sampler_create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_create_info.addressModeU  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_create_info.addressModeV =  VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_create_info.addressModeW =  VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_create_info.mipLodBias = 0.0f;
    sampler_create_info.anisotropyEnable = VK_TRUE;
    sampler_create_info.maxAnisotropy = 16;
    sampler_create_info.compareEnable = VK_FALSE;
    sampler_create_info.compareOp = VK_COMPARE_OP_ALWAYS;
    sampler_create_info.minLod = 0.0f;
    sampler_create_info.maxLod = 0.0f;
    sampler_create_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_create_info.unnormalizedCoordinates = VK_FALSE;
    
    VkResult result = vkCreateSampler(_device->_logical_device, &sampler_create_info, nullptr, &_sampler);
    ASSERT_VULKAN(result);
}

void texture_3d::create_image_view( VkImage image, VkFormat format, VkImageView& image_view)
{
    VkImageViewCreateInfo image_view_create_info {};
    
    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.pNext = nullptr;
    image_view_create_info.flags = 0;
    image_view_create_info.image = image;
    image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_3D;
    image_view_create_info.format = format;
    image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.subresourceRange.aspectMask = _aspect_flag;
    image_view_create_info.subresourceRange.baseMipLevel = 0;
    image_view_create_info.subresourceRange.levelCount = 1;
    image_view_create_info.subresourceRange.baseArrayLayer = 0;
    image_view_create_info.subresourceRange.layerCount = 1;
    
    VkResult result = vkCreateImageView(_device->_logical_device, &image_view_create_info, nullptr, &image_view);
    ASSERT_VULKAN(result);
}
