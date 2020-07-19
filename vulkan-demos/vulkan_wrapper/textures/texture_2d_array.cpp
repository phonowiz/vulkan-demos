//
//  texture_3d_array.cpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 11/6/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#include "texture_2d_array.h"
#include <cmath>

using namespace vk;

void texture_2d_array::init()
{
    
    _mip_levels = _enable_mipmapping ? static_cast<uint32_t>( std::floor(std::log2( std::max( _width, _height)))) + 1 : 1;
    create_sampler();
    
    create_image(
                 static_cast<VkFormat>(_format),
                 VK_IMAGE_TILING_OPTIMAL,
                 VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    
    create_image_view(_image, static_cast<VkFormat>(_format),_image_view);
    
    if(_enable_mipmapping)
        refresh_mimaps();
    
    _initialized = true;
}

VkImageCreateInfo texture_2d_array::get_image_create_info( VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage_flags)
{
    VkImageCreateInfo image_create_info = {};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.pNext  = nullptr;
    image_create_info.flags = 0;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = format;
    image_create_info.extent.width = _width;
    image_create_info.extent.height = _height;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = _mip_levels;
    image_create_info.arrayLayers = _depth;
    image_create_info.samples = _multisampling ? _device->get_max_usable_sample_count() :  VK_SAMPLE_COUNT_1_BIT;
    image_create_info.tiling = tiling;
    image_create_info.usage = usage_flags;
    
    return image_create_info;
}

void texture_2d_array::create_sampler()
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
    sampler_create_info.anisotropyEnable = VK_FALSE;
    sampler_create_info.maxAnisotropy = 8;
    sampler_create_info.compareEnable = VK_FALSE;
    sampler_create_info.compareOp = VK_COMPARE_OP_NEVER;
    sampler_create_info.minLod = 0.0f;
    sampler_create_info.maxLod = static_cast<float>(_mip_levels);
    //TODO: following needs to change according to texture data type
    sampler_create_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    sampler_create_info.unnormalizedCoordinates = VK_FALSE;
    
    VkResult result = vkCreateSampler(_device->_logical_device, &sampler_create_info, nullptr, &_sampler);
    ASSERT_VULKAN(result);
}

void texture_2d_array::create_image_view( VkImage image, VkFormat format, VkImageView& image_view)
{
    VkImageViewCreateInfo image_view_create_info {};
    
    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.pNext = nullptr;
    image_view_create_info.flags = 0;
    image_view_create_info.image = image;
    image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    image_view_create_info.format = format;
    image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.subresourceRange.aspectMask = _aspect_flag;
    image_view_create_info.subresourceRange.baseMipLevel = 0;
    image_view_create_info.subresourceRange.levelCount = _mip_levels;
    image_view_create_info.subresourceRange.baseArrayLayer = 0;
    image_view_create_info.subresourceRange.layerCount = _depth;
    
    VkResult result = vkCreateImageView(_device->_logical_device, &image_view_create_info, nullptr, &image_view);
    ASSERT_VULKAN(result);
}
