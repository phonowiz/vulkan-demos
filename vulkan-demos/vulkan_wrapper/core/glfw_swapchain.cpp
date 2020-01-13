//
//  swap_chain.cpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 2/7/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#include "glfw_swapchain.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>
#include <fstream>
#include <iostream>
#include <array>


using namespace vk;



glfw_swapchain::glfw_swapchain(device* device, GLFWwindow* window, VkSurfaceKHR surface)
{
    _device = device;
    _window =  window;
    //_swapchain_data.image_set.set_device(device);
    
    _surface = surface;
    
    recreate_swapchain();
    for( int i =0; i < NUM_SWAPCHAIN_IMAGES; ++i)
    {
        present_textures[0][i].set_device(device);
        present_textures[0][i].set_window(window);
        present_textures[0][i].set_swapchain(this);
        present_textures[0][i].set_swapchain_image_index(i);
        present_textures[0][i].set_dimensions(get_vk_swap_extent().width, get_vk_swap_extent().height, 1);
        present_textures[0][i].init();
    }

    
}
VkSurfaceFormatKHR glfw_swapchain::get_vk_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& available_formats)
{
    if (available_formats.size() == 1 && available_formats[0].format == VK_FORMAT_UNDEFINED) {
        return{VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
    }

    for (const auto& available_format : available_formats) {
        if (available_format.format == VK_FORMAT_B8G8R8A8_UNORM && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return available_format;
        }
    }

    return available_formats[0];
}

VkPresentModeKHR glfw_swapchain::get_vk_swap_present_mode(const std::vector<VkPresentModeKHR>& available_present_modes)
{
    VkPresentModeKHR best_mode = VK_PRESENT_MODE_FIFO_KHR;

    for (const auto& available_present_mode : available_present_modes) {
        if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return available_present_mode;
        } else if (available_present_mode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
            best_mode = available_present_mode;
        }
    }

    return best_mode;
}

VkExtent2D glfw_swapchain::get_vk_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow& window)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }
    else
    {
        //TODO: maybe is possible to eliminate the glfw library dependency from the vulkan wrapper
        int width, height;
        glfwGetFramebufferSize(&window, &width, &height);

        VkExtent2D actual_extent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actual_extent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actual_extent.width));
        actual_extent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actual_extent.height));

        return actual_extent;
    }
}

void glfw_swapchain::destroy_swapchain()
{
    VkSwapchainKHR old_swapchain = _swapchain;//swapchain_data.swapchain;
    vkDestroySwapchainKHR(_device->_logical_device, old_swapchain, nullptr);
}

//VkSurfaceFormatKHR swapchain::get_surface_format()
//{
//    device::swapchain_support_details swapchain_support;
//    _device->query_swapchain_support( _device->_physical_device, _surface, swapchain_support);
//
//    VkSurfaceFormatKHR surface_format = choose_swap_surface_format(swapchain_support.formats);
//    return surface_format;
//}

VkExtent2D glfw_swapchain::get_vk_swap_extent()
{
    device::swapchain_support_details swapchain_support;
    _device->query_swapchain_support( _device->_physical_device, _surface, swapchain_support);
    VkExtent2D extent = get_vk_swap_extent(swapchain_support.capabilities, *_window);
    
    return extent;
    
}
void glfw_swapchain::create_swapchain()
{
    device::swapchain_support_details swapchain_support;
    _device->query_swapchain_support( _device->_physical_device, _surface, swapchain_support);
    
    //all present textures have the same format and present modes
    VkSurfaceFormatKHR surface_format = get_vk_swap_surface_format(swapchain_support.formats);
    VkPresentModeKHR present_mode = get_vk_swap_present_mode(swapchain_support.presentModes);
    VkExtent2D extent = get_vk_swap_extent(swapchain_support.capabilities, *_window);
//    extent.width = present_textures[0][0].get_width();
//    extent.height = present_textures[0][0].get_height();
    
    
    uint32_t image_count = swapchain_support.capabilities.minImageCount + 1;
    if (swapchain_support.capabilities.maxImageCount > 0 && image_count > swapchain_support.capabilities.maxImageCount)
    {
        image_count = swapchain_support.capabilities.maxImageCount;
    }
    
    //assert(image_count == NUM_SWAPCHAIN_IMAGES && "other code depends on this number to be correct, please consider matching");
    
    VkSwapchainCreateInfoKHR create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = _surface;
    
    create_info.minImageCount = NUM_SWAPCHAIN_IMAGES;
    create_info.imageFormat = surface_format.format;
    create_info.imageColorSpace = surface_format.colorSpace;
    create_info.imageExtent = extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    
    device::queue_family_indices indices = _device->find_queue_families(_device->_physical_device, _surface);
    uint32_t queue_family_indices[] = {indices.graphics_family.value(), indices.present_family.value()};
    
    if (indices.graphics_family != indices.present_family)
    {
        assert(0 && "this path has not been tested, proceed with caution, even after this function runs, make sure depth buffer follows this sharing mode");
        create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices = queue_family_indices;
    }
    else
    {
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    
    create_info.preTransform = swapchain_support.capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = present_mode;
    create_info.clipped = VK_TRUE;
    
    
    if (vkCreateSwapchainKHR(_device->_logical_device, &create_info, nullptr, &(_swapchain)) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }
    
//    _swapchain_data.image_set.init(_device, _swapchain_data.swapchain);
//    _swapchain_data.image_set.create_image_set();
//    _swapchain_data.swapchain_extent = extent;
}

void glfw_swapchain::create_image_views()
{
    //_swapchain_data.image_set.create_image_views( get_surface_format().format );
}

void glfw_swapchain::print_stats()
{
    VkSurfaceCapabilitiesKHR surface_capabilities {};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_device->_physical_device, _surface, &surface_capabilities);
    std::cout << "Surface capabilities: " << std::endl;
    std::cout << "\tminImageCount: " << surface_capabilities.minImageCount << std::endl;
    std::cout << "\tmaxImageCount: " << surface_capabilities.maxImageCount << std::endl;
    std::cout << "\tcurrentExtent: " << surface_capabilities.currentExtent.width << "/" << surface_capabilities.currentExtent.height << std::endl;
    std::cout << "\tminImageExtent: " << surface_capabilities.minImageExtent.width << "/" << surface_capabilities.minImageExtent.height << std::endl;
    std::cout << "\tmaxImageExtent: " << surface_capabilities.maxImageExtent.width << "/" << surface_capabilities.maxImageExtent.height << std::endl;
    std::cout << "\tmaxImageArrayLayers: " << surface_capabilities.maxImageArrayLayers << std::endl;
    std::cout << "\tsupportedTransforms: " << surface_capabilities.supportedTransforms << std::endl;
    std::cout << "\tcurrentTransform: " << surface_capabilities.currentTransform << std::endl;
    std::cout << "\tsupportedCompositeAlpha: " << surface_capabilities.supportedCompositeAlpha << std::endl;
    std::cout << "\tsupportedUsageFlags: " << surface_capabilities.supportedUsageFlags << std::endl;
    
    uint32_t amountOfFormats = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(_device->_physical_device, _surface, &amountOfFormats, nullptr);
    
    assert( amountOfFormats < 100);
    auto surfaceFormats = std::array<VkSurfaceFormatKHR, 100>();
    vkGetPhysicalDeviceSurfaceFormatsKHR(_device->_physical_device, _surface, &amountOfFormats, surfaceFormats.data());
    
    std::cout << std::endl;
    std::cout << "Amount of Formats: " << amountOfFormats << std::endl;
    for (int i = 0; i < amountOfFormats; i++) {
        std::cout << "Format: " << surfaceFormats[i].format << std::endl;
        std::cout << "Color Space: " << surfaceFormats[i].colorSpace << std::endl;
    }
    
    uint32_t amountOfPresentationModes = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(_device->_physical_device, _surface, &amountOfPresentationModes, nullptr);
    assert( amountOfPresentationModes < 100);
    auto presentModes =std::array<VkPresentModeKHR, 100>(); 
    vkGetPhysicalDeviceSurfacePresentModesKHR(_device->_physical_device, _surface, &amountOfPresentationModes, presentModes.data());
    
    std::cout << std::endl;
    std::cout << "Amount of Presentation Modes: " << amountOfPresentationModes << std::endl;
    for (int i = 0; i < amountOfPresentationModes; i++) {
        std::cout << "Supported presentation mode: " << presentModes[i] << std::endl;
    }
    
}

void glfw_swapchain::recreate_swapchain( )
{
    _device->wait_for_all_operations_to_finish();
    
    VkSwapchainKHR old_swapchain = _swapchain;//_swapchain_data.swapchain;

    create_swapchain();
    //create_image_views();

    vkDestroySwapchainKHR(_device->_logical_device, old_swapchain, nullptr);
}

void glfw_swapchain::destroy()
{
    //_swapchain_data.image_set.destroy();
    vkDestroySwapchainKHR(_device->_logical_device, /*_swapchain_data.swapchain*/_swapchain, nullptr);
    /*_swapchain_data.swapchain*/_swapchain = VK_NULL_HANDLE;
}
glfw_swapchain::~glfw_swapchain()
{
}
