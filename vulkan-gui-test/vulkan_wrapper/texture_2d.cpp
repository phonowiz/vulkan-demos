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


const std::string texture_2d::textureResourcePath =  "/textures/";

texture_2d::texture_2d(device* device, uint32_t width, uint32_t height):
image(device)
{
    create_sampler();
    create(width, height);
}

texture_2d::texture_2d(device* device,const char* path)
:image(device)
{
    _path = resource::resource_root + texture_2d::textureResourcePath + path;
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
    VkSamplerCreateInfo samplerCreateInfo;
    
    samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerCreateInfo.pNext = nullptr;
    samplerCreateInfo.flags = 0;
    samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
    samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
    samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerCreateInfo.addressModeU  = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCreateInfo.addressModeV =  VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCreateInfo.addressModeW =  VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCreateInfo.mipLodBias = 0.0f;
    samplerCreateInfo.anisotropyEnable = VK_TRUE;
    samplerCreateInfo.maxAnisotropy = 16;
    samplerCreateInfo.compareEnable = VK_FALSE;
    samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerCreateInfo.minLod = 0.0f;
    samplerCreateInfo.maxLod = 0.0f;
    samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
    
    VkResult result = vkCreateSampler(_device->_logical_device, &samplerCreateInfo, nullptr, &_sampler);
    ASSERT_VULKAN(result);
}
void texture_2d::create(uint32_t width, uint32_t height)
{
    
    _width = width;
    _height = height;
    VkDeviceSize imageSize = get_size_in_bytes();


    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    
    
    create_buffer(_device->_logical_device, _device->_physical_device, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 stagingBuffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBufferMemory);
    
    if(_loaded)
    {
        void *data = nullptr;
        vkMapMemory(_device->_logical_device, stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, get_raw(), imageSize);
        vkUnmapMemory(_device->_logical_device, stagingBufferMemory);
    }
    
    create_image(_width,
                _height,
                static_cast<VkFormat>(_format),
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    

    change_image_layout(_device->_commandPool, _device->_graphics_queue, _image, static_cast<VkFormat>(_format),
                      VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    write_buffer_to_image(_device->_commandPool, _device->_graphics_queue, stagingBuffer);
    
    change_image_layout(_device->_commandPool, _device->_graphics_queue, _image, static_cast<VkFormat>(_format),
                      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    

    
    vkDestroyBuffer(_device->_logical_device, stagingBuffer, nullptr);
    vkFreeMemory(_device->_logical_device, stagingBufferMemory, nullptr);
    
    create_image_view(_image, static_cast<VkFormat>(_format), VK_IMAGE_ASPECT_COLOR_BIT, _imageView);
    _uploaded = true;
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
        vkDestroyImageView(_device->_logical_device, _imageView, nullptr);
        vkDestroyImage(_device->_logical_device, _image, nullptr);
        vkFreeMemory(_device->_logical_device, _imageMemory, nullptr);
        _sampler = VK_NULL_HANDLE;
        _imageView = VK_NULL_HANDLE;
        _image = VK_NULL_HANDLE;
        _imageMemory = VK_NULL_HANDLE;
        
        _uploaded = false;
    }
}
