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
#include "depth_texture.h"
#include "texture_2d.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "../textures/glfw_present_texture.h"

struct GLFWwindow;


namespace vk
{
    class glfw_swapchain : public object
    {
        
    public:
        
        static constexpr int NUM_SWAPCHAIN_IMAGES = 3;

        device* _device = nullptr;
        
        glfw_swapchain(device* device, GLFWwindow* window, VkSurfaceKHR surface);
        
        VkSurfaceKHR       get_vk_surface(){ return _surface; }
        void print_stats();
        
        VkSurfaceFormatKHR get_vk_surface_format()
        {
            device::swapchain_support_details swapchain_support;
            _device->query_swapchain_support( _device->_physical_device, get_vk_surface(), swapchain_support);
            
            VkSurfaceFormatKHR surface_format = get_vk_swap_surface_format(swapchain_support.formats);
            return surface_format;
        }
        
        GLFWwindow*   _window = nullptr;

        VkSurfaceFormatKHR  get_vk_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& availableFormats);
        VkPresentModeKHR    get_vk_swap_present_mode(const std::vector<VkPresentModeKHR>& availablePresentModes);
        VkExtent2D          get_vk_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow& window);
        
        VkExtent2D          get_vk_swap_extent();
        VkSwapchainKHR&      get_vk_swapchain() { return _swapchain; }
        void                create_swapchain();
        void                query_swapchain_support( device::swapchain_support_details& );
        void                destroy_swapchain();
        void                recreate_swapchain();
        
        std::array<std::array<glfw_present_texture, NUM_SWAPCHAIN_IMAGES>, 1> present_textures {};
        
        virtual void  destroy() override;
        ~glfw_swapchain();
        
    private:
        VkSurfaceKHR  _surface = VK_NULL_HANDLE;
        VkSwapchainKHR _swapchain = VK_NULL_HANDLE;
    };
}

