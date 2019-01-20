//
//  vulkan_utils.h
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 12/26/18.
//  Copyright © 2018 Rafael Sabino. All rights reserved.
//

#pragma once

#include "GLFW/glfw3.h"

#include <assert.h>
#include <vector>
#define ASSERT_VULKAN(val)\
if(val != VK_SUCCESS){\
assert(0);\
}


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

void createImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageView &imageView)
{
    
    VkImageViewCreateInfo imageViewCreateInfo;
    
    imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCreateInfo.pNext = nullptr;
    imageViewCreateInfo.flags = 0;
    imageViewCreateInfo.image = image;
    imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
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

void copyBuffer(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkBuffer src, VkBuffer dest, VkDeviceSize size);

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
    else
    {
        assert(0 && "transition not yet supported");
    }
    imageMemoryBarrier.oldLayout = oldLayout;
    imageMemoryBarrier.newLayout = newLayout;
    imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.image = image;
    imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
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

