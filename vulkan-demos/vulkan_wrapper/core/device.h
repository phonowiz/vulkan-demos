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
#include "EASTL/optional.h"
#include "EASTL/fixed_vector.h"
#include "object.h"


#define ASSERT_VULKAN(val)\
if(val != VK_SUCCESS){\
assert(0);\
}\
else{\
(void*)val;\
}

namespace vk {
    
    class device : public object
    {
    public:
        
        struct queue_family_indices {
            eastl::optional<uint32_t> graphics_family;
            eastl::optional<uint32_t> present_family;
            eastl::optional<uint32_t> compute_family;
            
            bool is_complete() {
                return graphics_family.has_value() && present_family.has_value() && compute_family.has_value();
            }
        };
        
        struct swapchain_support_details {
            VkSurfaceCapabilitiesKHR capabilities;
            eastl::fixed_vector<VkSurfaceFormatKHR, 20, true> formats;
            eastl::fixed_vector<VkPresentModeKHR, 20, true> presentModes;
        };
        
        void create_logical_device(VkSurfaceKHR surface);
        queue_family_indices find_queue_families( VkPhysicalDevice device, VkSurfaceKHR surface);
        
        
#ifndef __APPLE__
        const bool enableValidationLayers = true;
#else
        const bool enable_validation_layers = false;
#endif
        
        const eastl::fixed_vector<const char*, 20, true> device_extensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };
        
        bool check_device_extension_support(VkPhysicalDevice device);
        
        void query_swapchain_support( VkPhysicalDevice device, VkSurfaceKHR surface, swapchain_support_details& swapChainSupportDetails);
        void create_instance();
        void print_stats();
        void print_instance_layers();
        void print_instance_extensions();
        VkFormat find_depth_format();
        bool is_device_suitable( VkPhysicalDevice device, VkSurfaceKHR surface);
        
        VkFormat find_support_format( const eastl::fixed_vector<VkFormat, 20, true>& formats,
                                     VkImageTiling tiling, VkFormatFeatureFlags featureFlags);
        
        bool is_format_supported( VkFormat format,
                               VkImageTiling tiling, VkFormatFeatureFlags featureFlags);
        
        void pick_physical_device(VkSurfaceKHR surface);
        
        VkCommandBuffer start_single_time_command_buffer( VkCommandPool commandPool);
        void end_single_time_command_buffer(VkQueue queue, VkCommandPool commandPool, VkCommandBuffer commandBuffer);
        
        void copy_buffer( VkCommandPool commandPool, VkQueue queue, VkBuffer src, VkBuffer dest, VkDeviceSize size);
        void create_command_pool(uint32_t queueIndex, VkCommandPool* pool);
        void wait_for_all_operations_to_finish();
        VkPhysicalDeviceProperties get_properties() { return _properties; }
        
        virtual void destroy() override;
        device();
        ~device();
        
        VkPhysicalDevice    _physical_device = VK_NULL_HANDLE;
        VkDevice            _logical_device = VK_NULL_HANDLE;
        VkInstance          _instance = VK_NULL_HANDLE;
        VkQueue             _graphics_queue = VK_NULL_HANDLE;
        VkQueue             _present_queue = VK_NULL_HANDLE;
        VkQueue             _compute_queue = VK_NULL_HANDLE;
        VkCommandPool       _graphics_command_pool = VK_NULL_HANDLE;
        VkCommandPool       _present_command_pool = VK_NULL_HANDLE;
        VkCommandPool       _compute_command_pool = VK_NULL_HANDLE;
        VkPhysicalDeviceProperties _properties {};
        device::queue_family_indices _queue_family_indices;
        VkDebugReportCallbackEXT _callback {};
    private:
    };
}
