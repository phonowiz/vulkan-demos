//
//  depth_image.cpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 2/17/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#include "depth_image.h"

using namespace vk;

void depth_image::create(uint32_t width, uint32_t height)
{
    assert(!_created);
    
    VkFormat depth_format = _device->find_depth_format( );
    
    _width = width;
    _height = height;
    
    VkImageUsageFlagBits usage_flags = _write_to_texture ? static_cast<VkImageUsageFlagBits>(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT) :
                            static_cast<VkImageUsageFlagBits>(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
    
    create_image( depth_format,
                VK_IMAGE_TILING_OPTIMAL, usage_flags,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    
    create_image_view( _image, depth_format, VK_IMAGE_ASPECT_DEPTH_BIT, _image_view);
    
    change_image_layout(_device->_graphics_command_pool, _device->_graphics_queue, _image, depth_format, VK_IMAGE_LAYOUT_UNDEFINED,
                      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    
    create_sampler();
    _created = true;
}
void depth_image::create_sampler()
{
    VkSamplerCreateInfo sampler_create_info = {};
    
    sampler_create_info.magFilter = VK_FILTER_LINEAR;
    sampler_create_info.minFilter = VK_FILTER_LINEAR;
    sampler_create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_create_info.mipLodBias = 0.0f;
    sampler_create_info.maxAnisotropy = 1.0f;
    sampler_create_info.minLod = 0.0f;
    sampler_create_info.maxLod = 1.0f;
    sampler_create_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    
    VkResult result = vkCreateSampler(_device->_logical_device, &sampler_create_info, nullptr, &_sampler);
    ASSERT_VULKAN(result);
}

void depth_image::destroy()
{
    if(_created)
    {
        
        vkDestroyImageView(_device->_logical_device, _image_view, nullptr);
        vkDestroyImage(_device->_logical_device, _image, nullptr);
        vkFreeMemory(_device->_logical_device, _image_memory, nullptr);
        _created = false;
        _image = VK_NULL_HANDLE;
        _image_memory = VK_NULL_HANDLE;
        _image_view = VK_NULL_HANDLE;
    }
}

VkAttachmentDescription depth_image::get_depth_attachment()
{
    VkAttachmentDescription depth_attachment = {};
    
    depth_attachment.flags = 0;
    depth_attachment.format =_device->find_depth_format();
    depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp = _write_to_texture ?  VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    
    return depth_attachment;
}
