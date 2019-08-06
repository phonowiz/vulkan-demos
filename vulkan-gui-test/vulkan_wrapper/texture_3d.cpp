//
//  texture_3d.cpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 7/18/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#include "texture_3d.h"


using namespace vk;


void texture_3d::create(uint32_t width, uint32_t height, uint32_t depth)
{
    //if depth is 1, maybe this should be a 2d texture
    assert(width != 0 && height != 0 && depth > 1);
    _width = width;
    _height = height;
    _depth = depth;
    

}

void texture_3d::init()
{
    create_image(
                 static_cast<VkFormat>(_format),
                 VK_IMAGE_TILING_OPTIMAL,
                 VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    
    create_image_view(_image, static_cast<VkFormat>(_format), VK_IMAGE_ASPECT_COLOR_BIT,_image_view);
    
}

void texture_3d::create_sampler()
{
    VkSamplerCreateInfo sampler_create_info {};
    
    sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_create_info.pNext = nullptr;
    sampler_create_info.flags = 0;
    sampler_create_info.magFilter = VK_FILTER_LINEAR;
    sampler_create_info.minFilter = VK_FILTER_LINEAR;
    sampler_create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_create_info.addressModeU  = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_create_info.addressModeV =  VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_create_info.addressModeW =  VK_SAMPLER_ADDRESS_MODE_REPEAT;
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

void texture_3d::create_image_view( VkImage image, VkFormat format, VkImageAspectFlags aspect_flags, VkImageView& image_view)
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
    image_view_create_info.subresourceRange.aspectMask = aspect_flags;
    image_view_create_info.subresourceRange.baseMipLevel = 0;
    image_view_create_info.subresourceRange.levelCount = 1;
    image_view_create_info.subresourceRange.baseArrayLayer = 0;
    image_view_create_info.subresourceRange.layerCount = 1;
    
    VkResult result = vkCreateImageView(_device->_logical_device, &image_view_create_info, nullptr, &image_view);
    ASSERT_VULKAN(result);
}
