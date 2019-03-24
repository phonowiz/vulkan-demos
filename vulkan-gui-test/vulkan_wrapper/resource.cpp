//
//  Resource.cpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 2/12/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#include "resource.h"
#include <assert.h>
#include <fstream>
#include <iostream>

#include "device.h"
using namespace vk;
#if __APPLE__
const std::string resource::resource_root = "../Resources";
#else
const std::string Resource::resourceRoot = ".";
#endif


void resource::read_file(std::string& fileContents, std::string& path)
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

void resource::create_buffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize deviceSize, VkBufferUsageFlags bufferUsageFlags, VkBuffer &buffer,
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
    memoryAllocateInfo.memoryTypeIndex = find_memory_type_index(physicalDevice,memoryRequirements.memoryTypeBits, memoryPropertyFlags);
    
    result = vkAllocateMemory(device, &memoryAllocateInfo, nullptr, &deviceMemory);
    ASSERT_VULKAN(result);
    vkBindBufferMemory(device, buffer, deviceMemory, 0);
    
}

uint32_t resource::find_memory_type_index( VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
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
