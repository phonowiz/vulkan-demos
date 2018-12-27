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

void copyBuffer(VkBuffer src, VkBuffer dest, VkDeviceSize size);

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
    
    copyBuffer(stagingBuffer, buffer, bufferSize);
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, statingBufferMemory, nullptr);
    
}

