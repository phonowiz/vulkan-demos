//
//  texture.cpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 2/10/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#include "texture_2d.h"
#include <assert.h>
#include <cmath>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace vk;


const eastl::fixed_string<char, 250> texture_2d::texture_resource_path =  "/textures/";

texture_2d::texture_2d(device* device):image(device)
{
}

texture_2d::texture_2d(device* device, uint32_t width, uint32_t height):
image(device)
{
    _width = width;
    _height = height;
}

void texture_2d::init()
{
    if(!_initialized)
    {
        assert(_device != nullptr);
        _mip_levels = _enable_mipmapping ? static_cast<uint32_t>( std::floor(std::log2( std::max( _width, _height)))) + 1 : 1;
        create_sampler();
        assert( _width != 0 && _height != 0);
        create(_width, _height);
        
        _initialized = true;
    }
}

texture_2d::texture_2d(device* device,const char* path)
:image(device)
{
    _path = resource::resource_root + texture_2d::texture_resource_path + path;
    load();
}

void texture_2d::load()
{
    EA_ASSERT(_loaded !=true);
    int w = 0;
    int h = 0;
    int c = 0;
    
    //TODO: any way to detecct what the pixel format is? stbi_load might always
    //use this format, but am not sure.
    _format = formats::R8G8B8A8_UNSIGNED_NORMALIZED;
    _image_layout = image_layouts::PREINITIALIZED;
    EA_ASSERT_MSG(_path.empty() == false, "texture path is empty");
    _ppixels = stbi_load(_path.c_str(), &w,
                         &h, &c, STBI_rgb_alpha);
    _width = static_cast<uint32_t>(w);
    _height = static_cast<uint32_t>(h);
    _channels = static_cast<uint32_t>(c);
    
    if(_channels == 3)
    {
        _format = formats::R8G8B8_UNSIGNED_NORMALIZED;
    }
    
    EA_ASSERT_FORMATTED(_ppixels != nullptr, ("Texture did not load: %s", _path.c_str()));
    _loaded = true;
}

void texture_2d::create_sampler()
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
    sampler_create_info.maxLod = static_cast<float>(_mip_levels);
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
    
    
    VkMemoryPropertyFlagBits flags = _path.empty() ? static_cast<VkMemoryPropertyFlagBits>(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) : VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    create_buffer(_device->_logical_device, _device->_physical_device, image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                  staging_buffer, flags, staging_buffer_memory);
    
    if(_loaded)
    {
        void *data = nullptr;
        VkResult res = vkMapMemory(_device->_logical_device, staging_buffer_memory, 0, VK_WHOLE_SIZE, 0, &data);
        ASSERT_VULKAN(res);
        memcpy(data, get_raw(), image_size);
        vkUnmapMemory(_device->_logical_device, staging_buffer_memory);
    }
    
    create_image(
                 static_cast<VkFormat>(_format),
                 VK_IMAGE_TILING_OPTIMAL,
                 VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    
    
    change_image_layout(_device->_graphics_command_pool, _device->_graphics_queue, _image, static_cast<VkFormat>(_format),
                        VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    write_buffer_to_image(_device->_graphics_command_pool, _device->_graphics_queue, staging_buffer);
    
    if( _mip_levels == 1)
    {
        change_image_layout(_device->_graphics_command_pool, _device->_graphics_queue, _image, static_cast<VkFormat>(_format),
                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }
    else
    {
        refresh_mimaps();
    }
    
    
    vkDestroyBuffer(_device->_logical_device, staging_buffer, nullptr);
    vkFreeMemory(_device->_logical_device, staging_buffer_memory, nullptr);
    
    create_image_view(_image, static_cast<VkFormat>(_format), _image_view);
    _initialized = true;
}

void texture_2d::create_image_view(VkImage image, VkFormat format, VkImageView& image_view)
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
    image_view_create_info.subresourceRange.aspectMask = _aspect_flag;
    image_view_create_info.subresourceRange.baseMipLevel = 0;
    image_view_create_info.subresourceRange.levelCount = _mip_levels;
    image_view_create_info.subresourceRange.baseArrayLayer = 0;
    image_view_create_info.subresourceRange.layerCount = _depth;
    assert(_depth == 1);
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
    
    if(_initialized)
    {
        image::destroy();
        _initialized = false;
    }
}


void texture_2d::generate_mipmaps(VkImage image, VkCommandBuffer& command_buffer,
                                   uint32_t width,  uint32_t height, uint32_t depth)
{
    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    
    int32_t mip_width = width;
    int32_t mip_height = height;
    int32_t mip_depth = depth;
    
    for (uint32_t i = 1; i < _mip_levels; i++) {
        
        //this barrier will transition the previous mip level to VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL.  Upon creation
        //the image is set to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL.  This is because when the blit happens,
        //the previous level must be src optimal
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.subresourceRange.layerCount = mip_depth;
        barrier.subresourceRange.levelCount = 1;
        
        vkCmdPipelineBarrier(command_buffer,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                             0, nullptr,
                             0, nullptr,
                             1, &barrier);
        
        //here we create the current mip level
        VkImageBlit blit = {};
        blit.srcOffsets[0] = {0, 0, 0};
        blit.srcOffsets[1] = {mip_width, mip_height, 1};
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        //starting layer
        blit.srcSubresource.baseArrayLayer = 0;
        //how many layers to copy
        blit.srcSubresource.layerCount = mip_depth;
        blit.dstOffsets[0] = {0, 0, 0};
        blit.dstOffsets[1] = { mip_width > 1 ? mip_width / 2 : 1, mip_height > 1 ? mip_height / 2 : 1, 1};
        
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = mip_depth > 1 ? mip_depth /2 : 1;
        
        vkCmdBlitImage(command_buffer,
                       image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       1, &blit,
                       VK_FILTER_LINEAR);
        
        //transfer the base miplevel to shader optimal
        
        
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        
        //finally we switch the current mip level to VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT for shader sampling
        vkCmdPipelineBarrier(command_buffer,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                             0, nullptr,
                             0, nullptr,
                             1, &barrier);
        
        if (mip_width > 1) mip_width /= 2;
        if (mip_height > 1) mip_height /= 2;
        if (mip_depth > 1 ) mip_depth /= 2;
    }
    
    for(uint32_t i = 0; i < _mip_levels; ++i)
    {
        barrier.subresourceRange.baseMipLevel = i;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        
        vkCmdPipelineBarrier(command_buffer,
                             VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                             0, nullptr,
                             0, nullptr,
                             1, &barrier);

    }
}
//the following code is based off of: https://vulkan-tutorial.com/Generating_Mipmaps
void texture_2d::generate_mipmaps(VkImage image, VkCommandPool command_pool, VkQueue queue,
                             int32_t width, int32_t height, int32_t depth)
{
    VkCommandBuffer command_buffer =
        _device->start_single_time_command_buffer(command_pool);
    
    generate_mipmaps(image, command_buffer, width, height, depth);
    
    _device->end_single_time_command_buffer(queue, command_pool, command_buffer);
    

    _image_layout = image_layouts::SHADER_READ_ONLY_OPTIMAL;
    
}
