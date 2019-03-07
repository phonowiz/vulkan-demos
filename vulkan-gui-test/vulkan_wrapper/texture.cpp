//
//  texture.cpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 2/10/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#include "texture.h"
#include <assert.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace vk;



Texture::Texture(PhysicalDevice* device, uint32_t width, uint32_t height):
Image(device)
{
    create(width, height);
}

Texture::Texture(PhysicalDevice* device,const char* path)
:Image(device)
{
    _path = path;
    
    load();
    create( _width, _height);
}

void Texture::load( )
{
    assert(_loaded !=true);
    int w = 0;
    int h = 0;
    int c = 0;
    
    assert(_path != nullptr);
    _ppixels = stbi_load(_path, &w,
                         &h, &c, STBI_rgb_alpha);
    _width = static_cast<uint32_t>(w);
    _height = static_cast<uint32_t>(h);
    _channels = static_cast<uint32_t>(c);
    
    assert(_ppixels != nullptr);
    _loaded = true;
}

void Texture::create(uint32_t width, uint32_t height)
{
    VkDeviceSize imageSize = getSizeInBytes();
    
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    

    createBuffer(_device->_device, _device->_physicalDevice, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 stagingBuffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBufferMemory);
    
    if(_loaded)
    {
        void *data = nullptr;
        vkMapMemory(_device->_device, stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, getRaw(), imageSize);
        vkUnmapMemory(_device->_device, stagingBufferMemory);
    }
    
    createImage(_width,
                _height,
                static_cast<VkFormat>(_format),
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    
    
    changeImageLayout(_device->_commandPool, _device->_graphicsQueue, _image, static_cast<VkFormat>(_format),
                      VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    writeBufferToImage(_device->_commandPool, _device->_graphicsQueue, stagingBuffer);
    
    changeImageLayout(_device->_commandPool, _device->_graphicsQueue, _image, static_cast<VkFormat>(_format),
                      VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    
    vkDestroyBuffer(_device->_device, stagingBuffer, nullptr);
    vkFreeMemory(_device->_device, stagingBufferMemory, nullptr);
    
    createImageView(_image, static_cast<VkFormat>(_format), VK_IMAGE_ASPECT_COLOR_BIT, _imageView);
    
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
    
    VkResult result = vkCreateSampler(_device->_device, &samplerCreateInfo, nullptr, &_sampler);
    ASSERT_VULKAN(result);
    _uploaded = true;
}
void Texture::destroy()
{
    if(_loaded)
    {
        stbi_image_free(_ppixels);
        _loaded = false;
    }
    
    if(_uploaded)
    {
        vkDestroySampler(_device->_device, _sampler, nullptr);
        vkDestroyImageView(_device->_device, _imageView, nullptr);
        vkDestroyImage(_device->_device, _image, nullptr);
        vkFreeMemory(_device->_device, _imageMemory, nullptr);
        _sampler = VK_NULL_HANDLE;
        _imageView = VK_NULL_HANDLE;
        _image = VK_NULL_HANDLE;
        _imageMemory = VK_NULL_HANDLE;
        
        _uploaded = false;
    }
}
