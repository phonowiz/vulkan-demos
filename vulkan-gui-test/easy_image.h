//
//  EasyImage.h
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 1/4/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#pragma once
#include "vulkan_utils.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

class EasyImage
{
    
private:
    int width;
    int height;
    int channels;
    stbi_uc *ppixels;
    
    bool loaded = false;
    bool uploaded = false;
    VkImage image;
    VkDeviceMemory imageMemory;
    VkImageView imageView;
    VkImageLayout imageLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
    VkDevice device;
    VkSampler sampler;
    
public:
    
    EasyImage():
    loaded(false),
    ppixels(nullptr),
    width(0),
    height(0),
    channels(0)
    {
    }
    
    EasyImage(const char* path)
    {
        load(path);
    }
    
    ~EasyImage()
    {
        destroy();
    }
    
    void load( const char* path)
    {
        assert(loaded !=true);
        ppixels = stbi_load(path, &width, &height, &channels, STBI_rgb_alpha);
        
        assert(ppixels != nullptr);
        loaded = true;
    }
    void upload( const VkDevice &device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool,
                VkQueue queue)
    {
        assert(loaded == true);
        assert( uploaded == false);


        this->device = device;
        VkDeviceSize imageSize = getSizeInBytes();

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;

        createBuffer(device, physicalDevice, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     stagingBuffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBufferMemory);

        void *data;
        vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, getRaw(), imageSize);
        vkUnmapMemory(device, stagingBufferMemory);

        createImage(device, physicalDevice, (uint32_t)getWidth(), (uint32_t)getHeight(), VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, imageMemory);


        changeLayout(device, commandPool, queue, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        writeBufferToImage(device, commandPool, queue, stagingBuffer);
        changeLayout(device, commandPool, queue, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);

        createImageView(device, image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, imageView);

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

        VkResult result = vkCreateSampler(device, &samplerCreateInfo, nullptr, &sampler);
        uploaded = true;
    }
    
    void changeLayout(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkImageLayout layout)
    {

        changeImageLayout(device, commandPool, queue, image, VK_FORMAT_R8G8B8A8_UNORM, imageLayout, layout);
        imageLayout = layout;
    }
    
    void writeBufferToImage(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkBuffer buffer)
    {
        
        VkCommandBuffer commandBuffer = startSingleTimeCommandBuffer(device, commandPool);


        VkBufferImageCopy bufferImageCopy;

        bufferImageCopy.bufferOffset = 0;
        bufferImageCopy.bufferRowLength = 0;
        bufferImageCopy.bufferImageHeight = 0;
        bufferImageCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        bufferImageCopy.imageSubresource.mipLevel = 0;
        bufferImageCopy.imageSubresource.baseArrayLayer = 0;
        bufferImageCopy.imageSubresource.layerCount = 1;
        bufferImageCopy.imageOffset = { 0, 0, 0};
        bufferImageCopy.imageExtent = { static_cast<uint32_t>(getWidth()),
                                        static_cast<uint32_t>(getHeight()),
                                    1};

        vkCmdCopyBufferToImage(commandBuffer, buffer, image,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bufferImageCopy);


        endSingleTimeCommandBuffer(device, queue, commandPool, commandBuffer);
    }
    
    EasyImage(const EasyImage &) = delete;
    EasyImage(EasyImage &&) = delete;
    EasyImage& operator=(const EasyImage &) = delete;
    EasyImage& operator=(EasyImage &&) = delete;
    
    void destroy()
    {
        if(loaded)
        {
            stbi_image_free(ppixels);
            loaded = false;
        }
        
        if( uploaded)
        {
            vkDestroySampler(device, sampler, nullptr);
            vkDestroyImageView(device, imageView, nullptr);
            vkDestroyImage(device, image, nullptr);
            vkFreeMemory(device, imageMemory, nullptr);
            
            uploaded = false;
        }
    }
    
    int getWidth()
    {
        assert(loaded);
        return width;
    }
    
    int getHeight()
    {
        assert(loaded);
        return height;
    }
    
    int getChannels()
    {
        assert(loaded);
        assert(channels == 4);
        return channels;
    }
    
    int getSizeInBytes()
    {
        assert(loaded);
        return getWidth() * getHeight() * getChannels();
    }
    
    stbi_uc * getRaw()
    {
        assert(loaded);
        return ppixels;
    }
    
    VkSampler getSampler()
    {
        assert(loaded);
        return sampler;
    }
    
    VkImageView getImageView()
    {
        assert(loaded);
        return imageView;
    }

};
