//
//  vulkan_utils.h
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 12/26/18.
//  Copyright © 2018 Rafael Sabino. All rights reserved.
//

#pragma once

#include "GLFW/glfw3.h"

#include <string>
#include <fstream>
#include <iostream>
#include <assert.h>
#include <vector>

#include "utils/util_init.hpp"

#define ASSERT_VULKAN(val)\
if(val != VK_SUCCESS){\
assert(0);\
}

//vulkan renderer
uint32_t findMemoryTypeIndex( VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    //for memory buffer intro go here:
    //https://vulkan-tutorial.com/Vertex_buffers/Vertex_buffer_creation
    VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
    
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &physicalDeviceMemoryProperties);
    
    int32_t result = -1;
    for( int32_t i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; ++i)
    {
        if((typeFilter & (1 << i)) && (physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & properties) ==
           properties)
        {
            result =  i;
        }
    }
    assert( result != -1 && "memory property not found");
    return result;
}
//vulkan renderer
bool isFormatSupported(VkPhysicalDevice physicalDevice,  VkFormat format, VkImageTiling tiling, VkFormatFeatureFlags featureFlags)
{
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProperties);
    
    if(tiling == VK_IMAGE_TILING_LINEAR &&
       (formatProperties.linearTilingFeatures & featureFlags) == featureFlags)
    {
        return true;
    }
    else if( tiling == VK_IMAGE_TILING_OPTIMAL &&
            (formatProperties.optimalTilingFeatures & featureFlags) == featureFlags)
    {
        return true;
    }
    return false;
    
}
//vulkan renderer
VkFormat findSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& formats,
                             VkImageTiling tiling, VkFormatFeatureFlags featureFlags)
{
    for( VkFormat format : formats)
    {
        if(isFormatSupported(physicalDevice, format, tiling, featureFlags))
        {
            return format;
        }
    }
    assert(0 && "no supported format found!");
    return VK_FORMAT_UNDEFINED;
}
//vulkan renderer
bool isStencilFormat(VkFormat format)
{
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

//vulkan renderer
void createBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize deviceSize, VkBufferUsageFlags bufferUsageFlags, VkBuffer &buffer,
                  VkMemoryPropertyFlags memoryPropertyFlags, VkDeviceMemory &deviceMemory)
{
    VkBufferCreateInfo bufferCreateInfo;
    
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.pNext = nullptr;
    bufferCreateInfo.flags = 0;
    bufferCreateInfo.size = deviceSize;
    bufferCreateInfo.usage = bufferUsageFlags;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferCreateInfo.queueFamilyIndexCount = 0;
    bufferCreateInfo.pQueueFamilyIndices = nullptr;
    
    VkResult result = vkCreateBuffer(device, &bufferCreateInfo, nullptr, &buffer);
    ASSERT_VULKAN(result);
    VkMemoryRequirements memoryRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memoryRequirements);
    
    VkMemoryAllocateInfo memoryAllocateInfo;
    memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocateInfo.pNext = nullptr;
    memoryAllocateInfo.allocationSize = memoryRequirements.size;
    memoryAllocateInfo.memoryTypeIndex = findMemoryTypeIndex(physicalDevice,memoryRequirements.memoryTypeBits, memoryPropertyFlags);
    
    result = vkAllocateMemory(device, &memoryAllocateInfo, nullptr, &deviceMemory);
    ASSERT_VULKAN(result);
    vkBindBufferMemory(device, buffer, deviceMemory, 0);
    
}
//texture class
void createImage(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
                 VkImageUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags, VkImage &image, VkDeviceMemory &imageMemory)

{
    
    VkImageCreateInfo imageCreateInfo = {};
    
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.pNext  = nullptr;
    imageCreateInfo.flags = 0;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = format;
    imageCreateInfo.extent.width = width;
    imageCreateInfo.extent.height = height;
    imageCreateInfo.extent.depth = 1;
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.tiling = tiling;
    imageCreateInfo.usage = usageFlags;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.queueFamilyIndexCount = 0;
    imageCreateInfo.pQueueFamilyIndices = nullptr;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
    
    VkResult result = vkCreateImage(device, &imageCreateInfo, nullptr, &image);
    ASSERT_VULKAN(result);
    VkMemoryRequirements memoryRequirements;
    vkGetImageMemoryRequirements(device, image, &memoryRequirements);
    
    VkMemoryAllocateInfo memoryAllocateInfo;
    memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocateInfo.pNext = nullptr;
    memoryAllocateInfo.allocationSize = memoryRequirements.size;
    memoryAllocateInfo.memoryTypeIndex = findMemoryTypeIndex(physicalDevice, memoryRequirements.memoryTypeBits,
                                                             propertyFlags);
    result = vkAllocateMemory(device, &memoryAllocateInfo, nullptr, &imageMemory);
    ASSERT_VULKAN(result);
    
    vkBindImageMemory(device, image, imageMemory, 0);

}

//texture class
void createImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageView &imageView)
{
    
    VkImageViewCreateInfo imageViewCreateInfo;
    
    imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCreateInfo.pNext = nullptr;
    imageViewCreateInfo.flags = 0;
    imageViewCreateInfo.image = image;
    imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCreateInfo.format = format;
    imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.subresourceRange.aspectMask = aspectFlags;
    imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
    imageViewCreateInfo.subresourceRange.levelCount = 1;
    imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    imageViewCreateInfo.subresourceRange.layerCount = 1;
    
    VkResult result = vkCreateImageView(device, &imageViewCreateInfo, nullptr, &imageView);
    ASSERT_VULKAN(result);
    

}
//////////////////////////////////vulkan renderer
void copyBuffer(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkBuffer src, VkBuffer dest, VkDeviceSize size);

//vulkan command
VkCommandBuffer startSingleTimeCommandBuffer(VkDevice device, VkCommandPool commandPool)
{
    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.pNext = nullptr;
    commandBufferAllocateInfo.commandPool = commandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer = {};
    VkResult result = vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &commandBuffer);
    ASSERT_VULKAN(result);

    VkCommandBufferBeginInfo commandBufferBeginInfo;
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    commandBufferBeginInfo.pNext = nullptr;
    commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    commandBufferBeginInfo.pInheritanceInfo = nullptr;

    result = vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);
    ASSERT_VULKAN(result);
    
    return commandBuffer;

}


//vulkan command
void endSingleTimeCommandBuffer( VkDevice device, VkQueue queue, VkCommandPool commandPool, VkCommandBuffer commandBuffer)
{
    VkResult result = vkEndCommandBuffer(commandBuffer);
    ASSERT_VULKAN(result);
    
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext  = nullptr;
    submitInfo.waitSemaphoreCount = 0;
    submitInfo.pWaitSemaphores = nullptr;
    submitInfo.pWaitDstStageMask = nullptr;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    submitInfo.signalSemaphoreCount = 0;
    submitInfo.pSignalSemaphores = nullptr;
    
    result = vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    ASSERT_VULKAN(result);
    
    vkQueueWaitIdle(queue);
    
    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}
//vulkan renderer
template<typename T>
void createAndUploadBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkQueue queue, VkCommandPool commandPool,
                           std::vector<T>& data, VkBufferUsageFlags usage, VkBuffer &buffer, VkDeviceMemory &deviceMemory)
{
    VkDeviceSize bufferSize = sizeof(T) * data.size();
    assert(data.size() != 0);
    VkBuffer stagingBuffer;
    VkDeviceMemory statingBufferMemory;
    
    createBuffer(device, physicalDevice, bufferSize,  VK_BUFFER_USAGE_TRANSFER_SRC_BIT, stagingBuffer,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, statingBufferMemory);
    
    void* rawData= nullptr;
    VkResult r = vkMapMemory(device, statingBufferMemory, 0, bufferSize, 0, &rawData);
    ASSERT_VULKAN(r);
    memcpy(rawData, data.data(), bufferSize);
    vkUnmapMemory(device, statingBufferMemory);
    
    
    createBuffer(device, physicalDevice, bufferSize, usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT, buffer,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, deviceMemory);
    
    copyBuffer(device, commandPool, queue, stagingBuffer, buffer, bufferSize);
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, statingBufferMemory, nullptr);
    
}


////image class
void changeImageLayout(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkImage image,
                       VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
{
    VkCommandBuffer commandBuffer = startSingleTimeCommandBuffer(device, commandPool);
    
    VkImageMemoryBarrier imageMemoryBarrier;
    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.pNext = nullptr;
    if( oldLayout == VK_IMAGE_LAYOUT_PREINITIALIZED &&
       newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    }
    else if( oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
            newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    }
    else if( oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
            newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        imageMemoryBarrier.srcAccessMask = 0;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                                            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }
    else
    {
        assert(0 && "transition not yet supported");
    }
    imageMemoryBarrier.oldLayout = oldLayout;
    imageMemoryBarrier.newLayout = newLayout;
    imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.image = image;
    if(newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        
        if( isStencilFormat(format))
        {
            imageMemoryBarrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    }
    else
    {
        imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }
    
    imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
    imageMemoryBarrier.subresourceRange.levelCount = 1;
    imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
    imageMemoryBarrier.subresourceRange.layerCount = 1;
    
    vkCmdPipelineBarrier(commandBuffer,
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, //operations in this pipeline stage should occur before the barrier
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, //operations in this pipeline stage will wait on the barrier
                         0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
    
    
    endSingleTimeCommandBuffer(device, queue, commandPool, commandBuffer);
    

}

void getBaseDir(std::string& baseDir ) {
    
#if (defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK))
    baseDir =  "../Resources/";
#else
    assert( 0 && "getBaseDir not implemented");
#endif
}


void readFile(std::string& fileContents, std::string& path)
{
    std::ifstream fileStream(path, std::ios::in);
    if (!fileStream.is_open()) {
        std::cerr << "Couldn't load compute shader '" + std::string(path) + "'." << std::endl;
        fileStream.close();
        assert(false);
    }
    std::cout << "Loading kernel source from file " << path << "..." << std::endl;
    std::string line = "";
    while (!fileStream.eof()) {
        std::getline(fileStream, line);
        fileContents.append(line + "\n");
    }
}

//material class
void init_shaders(VkDevice &device, VkPipelineShaderStageCreateInfo &shaderStage, VkShaderStageFlagBits shaderType, const std::string& shaderText,
                  const char* name="main")
{
    VkResult U_ASSERT_ONLY res;
    bool U_ASSERT_ONLY retVal;
    
    assert(shaderText.empty() == false);
    
    init_glslang();
    
    std::vector<unsigned int> vtx_spv;
    shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStage.pNext = NULL;
    shaderStage.pSpecializationInfo = NULL;
    shaderStage.flags = 0;
    shaderStage.stage = shaderType;
    shaderStage.pName = name;
    shaderStage.pSpecializationInfo = nullptr;
    
    retVal = GLSLtoSPV(shaderType, shaderText.c_str(), vtx_spv);
    assert(retVal);
    VkShaderModuleCreateInfo moduleCreateInfo;
    moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleCreateInfo.pNext = NULL;
    moduleCreateInfo.flags = 0;
    moduleCreateInfo.codeSize = vtx_spv.size() * sizeof(unsigned int);
    moduleCreateInfo.pCode = vtx_spv.data();
    
    
    res = vkCreateShaderModule(device, &moduleCreateInfo, NULL, &shaderStage.module);
    assert(res == VK_SUCCESS);
    
    finalize_glslang();
}

