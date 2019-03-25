//
//  swap_chain.hpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 2/7/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#pragma once

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include <array>

#include "device.h"
#include <vector>
#include "depth_image.h"
#include "texture_2d.h"
#include "swapchain_image_set.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>


struct GLFWwindow;


namespace vk
{
    class swapchain : public object
    {
        
    public:
        
        static const int MAX_SWAPCHAIN_IMAGES = 5;
        struct swapchain_data
        {
            VkSwapchainKHR          swapchain = VK_NULL_HANDLE;
            swapchain_image_set     image_set;
            VkExtent2D              swapchain_extent;
        };
        
        device* _device = nullptr;
        
        swapchain(device* device, GLFWwindow* window, VkSurfaceKHR surface);
        
        VkSurfaceFormatKHR get_surface_format();
        void print_stats();
        
        
        swapchain_data _swapchain_data;
        GLFWwindow*   _window = nullptr;
        VkSurfaceKHR  _surface = VK_NULL_HANDLE;
        
        VkSurfaceFormatKHR  choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& availableFormats);
        VkPresentModeKHR    choose_swap_present_mode(const std::vector<VkPresentModeKHR>& availablePresentModes);
        VkExtent2D          choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow& window);
        void                create_swapchain();
        void                query_swapchain_support( device::swapchain_support_details& );
        void                create_image_views();
        void                destroy_swapchain();
        void                recreate_swapchain();
        
        virtual void  destroy() override;
        ~swapchain();
        
    private:
    };
}

