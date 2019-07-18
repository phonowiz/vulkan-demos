//
//  texture.cpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 2/10/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#include "texture_2d.h"
#include <assert.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace vk;


const std::string texture_2d::texture_resource_path =  "/textures/";

texture_2d::texture_2d(device* device):image(device)
{
}

texture_2d::texture_2d(device* device, uint32_t width, uint32_t height):
image(device)
{
    create_sampler();
    create(width, height);
}

texture_2d::texture_2d(device* device,const char* path)
:image(device)
{
    _path = resource::resource_root + texture_2d::texture_resource_path + path;
    create_sampler();
    load();
    create( _width, _height);
}

void texture_2d::load( )
{
    assert(_loaded !=true);
    int w = 0;
    int h = 0;
    int c = 0;
    
    assert(_path.empty() == false);
    _ppixels = stbi_load(_path.c_str(), &w,
                         &h, &c, STBI_rgb_alpha);
    _width = static_cast<uint32_t>(w);
    _height = static_cast<uint32_t>(h);
    _channels = static_cast<uint32_t>(c);
    
    assert(_ppixels != nullptr);
    _loaded = true;
}

void texture_2d::create_sampler()
{
    VkSamplerCreateInfo sampler_create_info;
    
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
void texture_2d::create(uint32_t width, uint32_t height)
{
    
    assert(width != 0 && height != 0);
    _width = width;
    _height = height;
    _depth = 1u;
    VkDeviceSize image_size = get_size_in_bytes();


    VkBuffer staging_buffer {};
    VkDeviceMemory staging_buffer_memory {};
    
    
    create_buffer(_device->_logical_device, _device->_physical_device, image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 staging_buffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer_memory);
    
    if(_loaded)
    {
        void *data = nullptr;
        vkMapMemory(_device->_logical_device, staging_buffer_memory, 0, image_size, 0, &data);
        memcpy(data, get_raw(), image_size);
        vkUnmapMemory(_device->_logical_device, staging_buffer_memory);
    }
    
    create_image(
                static_cast<VkFormat>(_format),
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    

    change_image_layout(_device->_graphics_command_pool, _device->_graphics_queue, _image, static_cast<VkFormat>(_format),
                      VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    write_buffer_to_image(_device->_graphics_command_pool, _device->_graphics_queue, staging_buffer);
    
    change_image_layout(_device->_graphics_command_pool, _device->_graphics_queue, _image, static_cast<VkFormat>(_format),
                      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    

    
    vkDestroyBuffer(_device->_logical_device, staging_buffer, nullptr);
    vkFreeMemory(_device->_logical_device, staging_buffer_memory, nullptr);
    
    create_image_view(_image, static_cast<VkFormat>(_format), VK_IMAGE_ASPECT_COLOR_BIT, _image_view);
    _uploaded = true;
}

void texture_2d::create_image_view(VkImage image, VkFormat format, VkImageAspectFlags aspect_flags, VkImageView &image_view)
{
    VkImageViewCreateInfo image_view_create_info {};
    
    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.pNext = nullptr;
    image_view_create_info.flags = 0;
    image_view_create_info.image = image;
    image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
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

void texture_2d::destroy()
{
    if(_loaded)
    {
        stbi_image_free(_ppixels);
        _loaded = false;
    }
    
    if(_uploaded)
    {
        vkDestroySampler(_device->_logical_device, _sampler, nullptr);
        vkDestroyImageView(_device->_logical_device, _image_view, nullptr);
        vkDestroyImage(_device->_logical_device, _image, nullptr);
        vkFreeMemory(_device->_logical_device, _image_memory, nullptr);
        _sampler = VK_NULL_HANDLE;
        _image_view = VK_NULL_HANDLE;
        _image = VK_NULL_HANDLE;
        _image_memory = VK_NULL_HANDLE;
        
        _uploaded = false;
    }
}
