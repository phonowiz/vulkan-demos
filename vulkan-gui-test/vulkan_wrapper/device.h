//
//  physical_device.hpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 2/7/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//
#pragma once

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include <optional>
#include <vector>
#include "object.h"


#define ASSERT_VULKAN(val)\
if(val != VK_SUCCESS){\
assert(0);\
}

namespace vk {
    
    class device : public object
    {
    public:
        
        struct QueueFamilyIndices {
            std::optional<uint32_t> graphicsFamily;
            std::optional<uint32_t> presentFamily;
            
            bool isComplete() {
                return graphicsFamily.has_value() && presentFamily.has_value();
            }
        };
        
        struct SwapChainSupportDetails {
            VkSurfaceCapabilitiesKHR capabilities;
            std::vector<VkSurfaceFormatKHR> formats;
            std::vector<VkPresentModeKHR> presentModes;
        };
        
        void createLogicalDevice(VkSurfaceKHR surface);
        QueueFamilyIndices findQueueFamilies( VkPhysicalDevice device, VkSurfaceKHR surface);
        
        
#ifndef __APPLE__
        const bool enableValidationLayers = true;
#else
        const bool enableValidationLayers = false;
#endif
        
        const std::vector<const char*> deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };
        
        bool checkDeviceExtensionSupport(VkPhysicalDevice device);
        
        //todo: there should be another version of this function which doesn't take a device argument and queries the chosen physical devie
        void querySwapChainSupport( VkPhysicalDevice device, VkSurfaceKHR surface, SwapChainSupportDetails& swapChainSupportDetails);
        void createInstance();
        void printStats();
        void printInstanceLayers();
        void printInstanceExtensions();
        VkFormat findDepthFormat();
        bool isDeviceSuitable( VkPhysicalDevice device, VkSurfaceKHR surface);
        
        VkFormat findSupportedFormat( const std::vector<VkFormat>& formats,
                                     VkImageTiling tiling, VkFormatFeatureFlags featureFlags);
        
        bool isFormatSupported( VkFormat format,
                               VkImageTiling tiling, VkFormatFeatureFlags featureFlags);
        
        void pickPhysicalDevice(VkSurfaceKHR surface);
        
        VkCommandBuffer startSingleTimeCommandBuffer( VkCommandPool commandPool);
        void endSingleTimeCommandBuffer(VkQueue queue, VkCommandPool commandPool, VkCommandBuffer commandBuffer);
        
        void copyBuffer( VkCommandPool commandPool, VkQueue queue, VkBuffer src, VkBuffer dest, VkDeviceSize size);
        void createCommnadPool(uint32_t queueIndex);
        void waitForllOperationsToFinish();
        
        virtual void destroy() override;
        device();
        ~device();
        
        VkPhysicalDevice    _physical_device = VK_NULL_HANDLE;
        VkDevice            _logical_device = VK_NULL_HANDLE;
        VkInstance          _instance = VK_NULL_HANDLE;
        VkQueue             _graphics_queue = VK_NULL_HANDLE;
        VkQueue             _presentQueue = VK_NULL_HANDLE;
        VkCommandPool       _commandPool = VK_NULL_HANDLE;

    private:
        
    };
    
    using PhysicalDeviceSharedPtr = std::shared_ptr<device>;
}
