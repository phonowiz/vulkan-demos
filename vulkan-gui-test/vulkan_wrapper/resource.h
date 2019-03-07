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

namespace  vk
{
    class Resource : public object
    {
    public:
        static const std::string resourceRoot;
        
        virtual ~Resource(){}
        
        void readFile(std::string& fileContents, std::string& path);
        
        void createBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize deviceSize, VkBufferUsageFlags bufferUsageFlags, VkBuffer &buffer,
                     VkMemoryPropertyFlags memoryPropertyFlags, VkDeviceMemory &deviceMemory);
        
        uint32_t findMemoryTypeIndex( VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);
        
        
        enum class UsageType
        {
            COMBINED_IMAGE_SAMPLER = VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            UNIFORM_BUFFER = VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            INVALID = VkDescriptorType::VK_DESCRIPTOR_TYPE_MAX_ENUM
        };
        
        //todo: this should be protected
        struct BufferInfo
        {
            VkBuffer        uniformBuffer =         VK_NULL_HANDLE;
            VkDeviceMemory  uniformBufferMemory =   VK_NULL_HANDLE;
            UsageType       usageType =             UsageType::INVALID;
            size_t size = 0;
        };

    };
}

