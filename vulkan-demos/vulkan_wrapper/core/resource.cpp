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

#include "EASTL/fixed_string.h"
#include "EAAssert/eaassert.h"
#include "device.h"
using namespace vk;
#if __APPLE__
const eastl::fixed_string<char, 250> resource::resource_root = "../Resources";
#else
const std::string Resource::resourceRoot = ".";
#endif


void resource::read_file(std::string& fileContents, eastl::fixed_string<char, 250>& path)
{
    eastl::fixed_string<char, 200> msg {};
    std::ifstream fileStream(path.c_str(), std::ios::in);
    if (!fileStream.is_open()) {
        
        eastl::fixed_string<char, 200> msg;
        msg.sprintf("Couldn't load compute shader '%s'", path.c_str());
        std::cerr <<  msg.c_str() << std::endl;
        fileStream.close();
        EA_FAIL_FORMATTED(("%s was not found", path.c_str()));
    }
    msg.sprintf("Loading kernel source from file %s...", path.c_str());
    std::cout << msg.c_str() << std::endl;
    std::string line = "";
    while (!fileStream.eof()) {
        std::getline(fileStream, line);
        fileContents.append(line + "\n");
    }
}

#include "debug_utils.h"

void resource::create_buffer(VkDevice device, VkPhysicalDevice physical_device, VkDeviceSize device_size, VkBufferUsageFlags buffer_usage_flags, VkBuffer &buffer,
                         VkMemoryPropertyFlags memory_propery_flags, VkDeviceMemory &device_memory)
{
    VkBufferCreateInfo buffer_create_info = {};
    
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.pNext = nullptr;
    buffer_create_info.flags = 0;
    buffer_create_info.size = device_size;
    buffer_create_info.usage = buffer_usage_flags;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    buffer_create_info.queueFamilyIndexCount = 0;
    buffer_create_info.pQueueFamilyIndices = nullptr;
    
    VkResult result = vkCreateBuffer(device, &buffer_create_info, nullptr, &buffer);
    ASSERT_VULKAN(result);
    VkMemoryRequirements memory_requirements {};
    vkGetBufferMemoryRequirements(device, buffer, &memory_requirements);
    
    VkMemoryAllocateInfo memory_allocate_info = {};
    memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memory_allocate_info.pNext = nullptr;
    memory_allocate_info.allocationSize = memory_requirements.size;
    memory_allocate_info.memoryTypeIndex = find_memory_type_index(physical_device,memory_requirements.memoryTypeBits, memory_propery_flags);
    
    result = vkAllocateMemory(device, &memory_allocate_info, nullptr, &device_memory);
    
    ASSERT_VULKAN(result);
    vkBindBufferMemory(device, buffer, device_memory, 0);
    
}

uint32_t resource::find_memory_type_index( VkPhysicalDevice physical_device, uint32_t type_filter, VkMemoryPropertyFlags properties)
{
    //for memory buffer intro go here:
    //https://vulkan-tutorial.com/Vertex_buffers/Vertex_buffer_creation
    VkPhysicalDeviceMemoryProperties physical_device_mem_properties;
    
    vkGetPhysicalDeviceMemoryProperties(physical_device, &physical_device_mem_properties);
    
    int32_t result = -1;
    for( int32_t i = 0; i < physical_device_mem_properties.memoryTypeCount; ++i)
    {
        if((type_filter & (1 << i)) && (physical_device_mem_properties.memoryTypes[i].propertyFlags & properties) ==
           properties)
        {
            result =  i;
        }
    }
    assert( result != -1 && "memory property not found");
    return result;
}

void* resource::aligned_alloc(size_t size, size_t alignment)
{
    void *data = nullptr;
#if defined(_MSC_VER) || defined(__MINGW32__)
    data = _aligned_malloc(size, alignment);
#else
    int res = posix_memalign(&data, alignment, size);
    if (res != 0)
        data = nullptr;
#endif
    
    return data;
}

void resource::aligned_free(void* data)
{
#if    defined(_MSC_VER) || defined(__MINGW32__)
    _aligned_free(data);
#else
    free(data);
#endif
}
