//
//  Resource.hpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 2/12/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#pragma once

#include <string>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include "object.h"
#include <atomic>

namespace  vk
{
    class resource : public object
    {
    public:
        static const std::string resource_root;
        
        virtual ~resource(){}
        
        void read_file(std::string& fileContents, std::string& path);
        
        void create_buffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize deviceSize, VkBufferUsageFlags bufferUsageFlags, VkBuffer &buffer,
                     VkMemoryPropertyFlags memoryPropertyFlags, VkDeviceMemory &deviceMemory);
    
        void* aligned_alloc(size_t size, size_t alignment);
        void  aligned_free(void* data);
        
        uint32_t find_memory_type_index( VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);
    
        enum class usage_type
        {
            COMBINED_IMAGE_SAMPLER = VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            STORAGE_IMAGE = VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            UNIFORM_BUFFER = VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            DYNAMIC_UNIFORM_BUFFER = VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
            INPUT_ATTACHMENT = VkDescriptorType::VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
            INVALID = VkDescriptorType::VK_DESCRIPTOR_TYPE_MAX_ENUM
        };
        
    protected:
        struct buffer_info
        {
            VkBuffer        uniform_buffer =           VK_NULL_HANDLE;
            VkDeviceMemory  device_memory =   VK_NULL_HANDLE;
            void*           host_mem = nullptr;
            usage_type      usage_type =             usage_type::INVALID;
            uint32_t        binding   =             0;
            size_t          size      =             0;
        };

    };
}

