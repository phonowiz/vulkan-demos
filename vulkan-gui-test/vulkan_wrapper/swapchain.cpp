//
//  swap_chain.cpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 2/7/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#include "swapchain.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>
#include <fstream>
#include <iostream>
#include <array>


using namespace vk;



swapchain::swapchain(device* device, GLFWwindow* window, VkSurfaceKHR surface)
{
    _device = device;
    _window =  window;
    _swapchain_data.image_set.set_device(device);
    
    _surface = surface;
    
    recreate_swapchain();
    
}
VkSurfaceFormatKHR swapchain::choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& available_formats)
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

VkPresentModeKHR swapchain::choose_swap_present_mode(const std::vector<VkPresentModeKHR>& available_present_modes)
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

VkExtent2D swapchain::choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow& window)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }
    else
    {
        //todo: maybe is possible to eliminate the glfw library dependency from the vulkan wrapper
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

void swapchain::destroy_swapchain()
{
    VkSwapchainKHR old_swapchain = _swapchain_data.swapchain;
    vkDestroySwapchainKHR(_device->_logical_device, old_swapchain, nullptr);
}

VkSurfaceFormatKHR swapchain::get_surface_format()
{
    device::swapchain_support_details swapchain_support;
    _device->query_swapchain_support( _device->_physical_device, _surface, swapchain_support);
    
    VkSurfaceFormatKHR surface_format = choose_swap_surface_format(swapchain_support.formats);
    return surface_format;
}
void swapchain::create_swapchain()
{
    device::swapchain_support_details swapchain_support;
    _device->query_swapchain_support( _device->_physical_device, _surface, swapchain_support);
    
    VkSurfaceFormatKHR surface_format = choose_swap_surface_format(swapchain_support.formats);
    VkPresentModeKHR present_mode = choose_swap_present_mode(swapchain_support.presentModes);
    VkExtent2D extent = choose_swap_extent(swapchain_support.capabilities, *_window);
    
    uint32_t image_count = swapchain_support.capabilities.minImageCount + 1;
    if (swapchain_support.capabilities.maxImageCount > 0 && image_count > swapchain_support.capabilities.maxImageCount)
    {
        image_count = swapchain_support.capabilities.maxImageCount;
    }
    
    VkSwapchainCreateInfoKHR create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = _surface;
    
    create_info.minImageCount = image_count;
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
    
    
    if (vkCreateSwapchainKHR(_device->_logical_device, &create_info, nullptr, &(_swapchain_data.swapchain)) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }
    
    _swapchain_data.image_set.init(_device, _swapchain_data.swapchain);
    _swapchain_data.image_set.create_image_set();
    _swapchain_data.swapchain_extent = extent;
}

void swapchain::create_image_views()
{
    _swapchain_data.image_set.create_image_views( get_surface_format().format );
}

void swapchain::print_stats()
{
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_device->_physical_device, _surface, &surfaceCapabilities);
    std::cout << "Surface capabilities: " << std::endl;
    std::cout << "\tminImageCount: " << surfaceCapabilities.minImageCount << std::endl;
    std::cout << "\tmaxImageCount: " << surfaceCapabilities.maxImageCount << std::endl;
    std::cout << "\tcurrentExtent: " << surfaceCapabilities.currentExtent.width << "/" << surfaceCapabilities.currentExtent.height << std::endl;
    std::cout << "\tminImageExtent: " << surfaceCapabilities.minImageExtent.width << "/" << surfaceCapabilities.minImageExtent.height << std::endl;
    std::cout << "\tmaxImageExtent: " << surfaceCapabilities.maxImageExtent.width << "/" << surfaceCapabilities.maxImageExtent.height << std::endl;
    std::cout << "\tmaxImageArrayLayers: " << surfaceCapabilities.maxImageArrayLayers << std::endl;
    std::cout << "\tsupportedTransforms: " << surfaceCapabilities.supportedTransforms << std::endl;
    std::cout << "\tcurrentTransform: " << surfaceCapabilities.currentTransform << std::endl;
    std::cout << "\tsupportedCompositeAlpha: " << surfaceCapabilities.supportedCompositeAlpha << std::endl;
    std::cout << "\tsupportedUsageFlags: " << surfaceCapabilities.supportedUsageFlags << std::endl;
    
    uint32_t amountOfFormats = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(_device->_physical_device, _surface, &amountOfFormats, nullptr);
    
    assert( amountOfFormats < 100);
    auto surfaceFormats = std::array<VkSurfaceFormatKHR, 100>();//new VkSurfaceFormatKHR[amountOfFormats];
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
    auto presentModes =std::array<VkPresentModeKHR, 100>(); //new VkPresentModeKHR[amountOfPresentationModes];
    vkGetPhysicalDeviceSurfacePresentModesKHR(_device->_physical_device, _surface, &amountOfPresentationModes, presentModes.data());
    
    std::cout << std::endl;
    std::cout << "Amount of Presentation Modes: " << amountOfPresentationModes << std::endl;
    for (int i = 0; i < amountOfPresentationModes; i++) {
        std::cout << "Supported presentation mode: " << presentModes[i] << std::endl;
    }
    
}

void swapchain::recreate_swapchain( )
{
    _device->wait_for_all_operations_to_finish();
    
    VkSwapchainKHR oldSwapchain = _swapchain_data.swapchain;

    create_swapchain();
    create_image_views();

    vkDestroySwapchainKHR(_device->_logical_device, oldSwapchain, nullptr);
}

void swapchain::destroy()
{
    _swapchain_data.image_set.destroy();
    vkDestroySwapchainKHR(_device->_logical_device, _swapchain_data.swapchain, nullptr);
    _swapchain_data.swapchain = VK_NULL_HANDLE;
}
swapchain::~swapchain()
{
}
