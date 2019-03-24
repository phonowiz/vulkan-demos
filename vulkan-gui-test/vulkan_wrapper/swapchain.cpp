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



swapchain::swapchain(device* device, GLFWwindow* window, VkSurfaceKHR surface):
_depth_image(device)
{
    _device = device;
    _window =  window;
    _swapchain_data.image_set.set_device(device);
    
    _surface = surface;
    
}
VkSurfaceFormatKHR swapchain::choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED) {
        return{VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
    }
    
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }
    
    return availableFormats[0];
}

VkPresentModeKHR swapchain::choose_swap_present_mode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
    VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;
    
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        } else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
            bestMode = availablePresentMode;
        }
    }
    
    return bestMode;
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
        
        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };
        
        actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));
        
        return actualExtent;
    }
}

void swapchain::destroy_swapchain()
{
    VkSwapchainKHR oldSwapchain = _swapchain_data.swapchain;
    vkDestroySwapchainKHR(_device->_logical_device, oldSwapchain, nullptr);
}

VkSurfaceFormatKHR swapchain::get_surface_format()
{
    device::swapchain_support_details swapChainSupport;
    _device->query_swapchain_support( _device->_physical_device, _surface, swapChainSupport);
    
    VkSurfaceFormatKHR surfaceFormat = choose_swap_surface_format(swapChainSupport.formats);
    return surfaceFormat;
}
void swapchain::create_swapchain()
{
    device::swapchain_support_details swapChainSupport;
    _device->query_swapchain_support( _device->_physical_device, _surface, swapChainSupport);
    
    VkSurfaceFormatKHR surfaceFormat = choose_swap_surface_format(swapChainSupport.formats);
    VkPresentModeKHR presentMode = choose_swap_present_mode(swapChainSupport.presentModes);
    VkExtent2D extent = choose_swap_extent(swapChainSupport.capabilities, *_window);
    
    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
    {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }
    
    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = _surface;
    
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    
    device::queue_family_indices indices = _device->findQueueFamilies(_device->_physical_device, _surface);
    uint32_t queueFamilyIndices[] = {indices.graphics_family.value(), indices.present_family.value()};
    
    if (indices.graphics_family != indices.present_family)
    {
        assert(0 && "this path has not been tested, proceed with caution, even after this function runs, make sure depth buffer follows this sharing mode");
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    
    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    
    
    if (vkCreateSwapchainKHR(_device->_logical_device, &createInfo, nullptr, &(_swapchain_data.swapchain)) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }
    
    _swapchain_data.image_set.init(_device, _swapchain_data.swapchain);
    _swapchain_data.image_set.create_image_set();
    _swapchain_data.swapchain_extent = extent;
}


//todo: this function should be part of swapchain
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

void swapchain::create_frame_buffers(VkRenderPass renderPass)
{
    //_swapChainData.swapChainFramebuffers.resize(_swapChainData.swapChainImages.size());
    
    //TODO: Get rid of the vector class
    _swapchain_data.swapchain_frame_buffers.resize(_swapchain_data.image_set.get_image_count());

    for (size_t i = 0; i < _swapchain_data.swapchain_frame_buffers.size(); i++)
    {
        VkImageView depthImageView = _depth_image.get_image_view();
        std::array<VkImageView, 2> attachmentViews = {_swapchain_data.image_set.get_image_views()[i], depthImageView};
        
        VkFramebufferCreateInfo framebufferCreateInfo;
        framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCreateInfo.pNext = nullptr;
        framebufferCreateInfo.flags = 0;
        framebufferCreateInfo.renderPass = renderPass;
        framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachmentViews.size());
        framebufferCreateInfo.pAttachments = attachmentViews.data();
        framebufferCreateInfo.width = _swapchain_data.swapchain_extent.width;
        framebufferCreateInfo.height = _swapchain_data.swapchain_extent.height;
        framebufferCreateInfo.layers = 1;
        
        VkResult result = vkCreateFramebuffer(_device->_logical_device, &framebufferCreateInfo, nullptr, &(_swapchain_data.swapchain_frame_buffers[i]));
        ASSERT_VULKAN(result);
    }
}

void swapchain::create_depth_image()
{
    _depth_image.create(
                        _swapchain_data.swapchain_extent.width,
                        _swapchain_data.swapchain_extent.height
                       );
}

void swapchain::recreate_swapchain( VkRenderPass renderPass)
{
    _device->wait_for_all_operations_to_finish();
    
    _depth_image.destroy();
    
    for (size_t i = 0; i < _swapchain_data.swapchain_frame_buffers.size(); i++)
    {
        vkDestroyFramebuffer(_device->_logical_device, _swapchain_data.swapchain_frame_buffers[i], nullptr);
    }
    
    VkSwapchainKHR oldSwapchain = _swapchain_data.swapchain;

    create_swapchain();
    create_image_views();
    create_depth_image();
    create_frame_buffers(renderPass);

    vkDestroySwapchainKHR(_device->_logical_device, oldSwapchain, nullptr);
}

VkAttachmentDescription swapchain::get_depth_attachment()
{
    return _depth_image.getDepthAttachment();
}

void swapchain::destroy()
{
    _depth_image.destroy();
    
    for (size_t i = 0; i < _swapchain_data.swapchain_frame_buffers.size(); i++)
    {
        vkDestroyFramebuffer(_device->_logical_device, _swapchain_data.swapchain_frame_buffers[i], nullptr);
        _swapchain_data.swapchain_frame_buffers[i] = VK_NULL_HANDLE;
    }
    
    _swapchain_data.image_set.destroy();
    vkDestroySwapchainKHR(_device->_logical_device, _swapchain_data.swapchain, nullptr);
    _swapchain_data.swapchain = VK_NULL_HANDLE;
}
swapchain::~swapchain()
{
}
