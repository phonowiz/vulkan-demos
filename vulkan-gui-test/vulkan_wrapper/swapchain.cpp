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



swapchain::swapchain(device* physicalDevice, GLFWwindow* window):
_depthImage(physicalDevice)
{
    _physicalDevice = physicalDevice;
    _window =  window;
    _swapChainData.imageSet.set_device(physicalDevice);
    
    //todo: following call must go outside of this class
    VkResult result = glfwCreateWindowSurface(_physicalDevice->_instance, _window, nullptr, &_surface);
    ASSERT_VULKAN(result);
    
}
VkSurfaceFormatKHR swapchain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
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

VkPresentModeKHR swapchain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
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

VkExtent2D swapchain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow& window)
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

void swapchain::destroySwapChain()
{
    VkSwapchainKHR oldSwapchain = _swapChainData.swapChain;
    vkDestroySwapchainKHR(_physicalDevice->_logical_device, oldSwapchain, nullptr);
}

VkSurfaceFormatKHR swapchain::getSurfaceFormat()
{
    device::swapchain_support_details swapChainSupport;
    _physicalDevice->query_swapchain_support( _physicalDevice->_physical_device, _surface, swapChainSupport);
    
    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    return surfaceFormat;
}
void swapchain::createSwapChain()
{
    device::swapchain_support_details swapChainSupport;
    _physicalDevice->query_swapchain_support( _physicalDevice->_physical_device, _surface, swapChainSupport);
    
    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities, *_window);
    
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
    
    device::queue_family_indices indices = _physicalDevice->findQueueFamilies(_physicalDevice->_physical_device, _surface);
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
    
    
    if (vkCreateSwapchainKHR(_physicalDevice->_logical_device, &createInfo, nullptr, &(_swapChainData.swapChain)) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }
    
    _swapChainData.imageSet.init(_physicalDevice, _swapChainData.swapChain);
    _swapChainData.imageSet.createImageSet();
    _swapChainData.swapChainExtent = extent;
}

void swapchain::createSurface()
{
    
    if (glfwCreateWindowSurface(_physicalDevice->_instance, _window, nullptr, &_surface) != VK_SUCCESS)
    {
        assert( 0 && "couldn't create surface");
    }
}

//todo: this function should be part of swapchain
void swapchain::createImageViews()
{
    _swapChainData.imageSet.createImageViews( getSurfaceFormat().format );
}

void swapchain::printStats()
{
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_physicalDevice->_physical_device, _surface, &surfaceCapabilities);
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
    vkGetPhysicalDeviceSurfaceFormatsKHR(_physicalDevice->_physical_device, _surface, &amountOfFormats, nullptr);
    
    assert( amountOfFormats < 100);
    auto surfaceFormats = std::array<VkSurfaceFormatKHR, 100>();//new VkSurfaceFormatKHR[amountOfFormats];
    vkGetPhysicalDeviceSurfaceFormatsKHR(_physicalDevice->_physical_device, _surface, &amountOfFormats, surfaceFormats.data());
    
    std::cout << std::endl;
    std::cout << "Amount of Formats: " << amountOfFormats << std::endl;
    for (int i = 0; i < amountOfFormats; i++) {
        std::cout << "Format: " << surfaceFormats[i].format << std::endl;
        std::cout << "Color Space: " << surfaceFormats[i].colorSpace << std::endl;
    }
    
    uint32_t amountOfPresentationModes = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(_physicalDevice->_physical_device, _surface, &amountOfPresentationModes, nullptr);
    assert( amountOfPresentationModes < 100);
    auto presentModes =std::array<VkPresentModeKHR, 100>(); //new VkPresentModeKHR[amountOfPresentationModes];
    vkGetPhysicalDeviceSurfacePresentModesKHR(_physicalDevice->_physical_device, _surface, &amountOfPresentationModes, presentModes.data());
    
    std::cout << std::endl;
    std::cout << "Amount of Presentation Modes: " << amountOfPresentationModes << std::endl;
    for (int i = 0; i < amountOfPresentationModes; i++) {
        std::cout << "Supported presentation mode: " << presentModes[i] << std::endl;
    }
    
}

void swapchain::createFrameBuffers(VkRenderPass renderPass)
{
    //_swapChainData.swapChainFramebuffers.resize(_swapChainData.swapChainImages.size());
    
    //TODO: Get rid of the vector class
    _swapChainData.swapChainFramebuffers.resize(_swapChainData.imageSet.getImageCount());

    for (size_t i = 0; i < _swapChainData.swapChainFramebuffers.size(); i++)
    {
        VkImageView depthImageView = _depthImage.get_image_view();
        std::array<VkImageView, 2> attachmentViews = {_swapChainData.imageSet.getImageViews()[i], depthImageView};
        
        VkFramebufferCreateInfo framebufferCreateInfo;
        framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCreateInfo.pNext = nullptr;
        framebufferCreateInfo.flags = 0;
        framebufferCreateInfo.renderPass = renderPass;
        framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachmentViews.size());
        framebufferCreateInfo.pAttachments = attachmentViews.data();
        framebufferCreateInfo.width = _swapChainData.swapChainExtent.width;
        framebufferCreateInfo.height = _swapChainData.swapChainExtent.height;
        framebufferCreateInfo.layers = 1;
        
        VkResult result = vkCreateFramebuffer(_physicalDevice->_logical_device, &framebufferCreateInfo, nullptr, &(_swapChainData.swapChainFramebuffers[i]));
        ASSERT_VULKAN(result);
    }
}

void swapchain::createDepthImage()
{
    _depthImage.create(
                        _swapChainData.swapChainExtent.width,
                        _swapChainData.swapChainExtent.height
                       );
}

void swapchain::recreateSwapChain( VkRenderPass renderPass)
{
    _physicalDevice->wait_for_all_operations_to_finish();
    
    _depthImage.destroy();
    
    for (size_t i = 0; i < _swapChainData.swapChainFramebuffers.size(); i++)
    {
        vkDestroyFramebuffer(_physicalDevice->_logical_device, _swapChainData.swapChainFramebuffers[i], nullptr);
    }
    
    VkSwapchainKHR oldSwapchain = _swapChainData.swapChain;

    createSwapChain();
    createImageViews();
    createDepthImage();
    createFrameBuffers(renderPass);

    vkDestroySwapchainKHR(_physicalDevice->_logical_device, oldSwapchain, nullptr);
}

VkAttachmentDescription swapchain::getDepthAttachment()
{
    return _depthImage.getDepthAttachment();
}

void swapchain::destroy()
{
    _depthImage.destroy();
    
    for (size_t i = 0; i < _swapChainData.swapChainFramebuffers.size(); i++)
    {
        vkDestroyFramebuffer(_physicalDevice->_logical_device, _swapChainData.swapChainFramebuffers[i], nullptr);
        _swapChainData.swapChainFramebuffers[i] = VK_NULL_HANDLE;
    }
    
    _swapChainData.imageSet.destroy();
    vkDestroySwapchainKHR(_physicalDevice->_logical_device, _swapChainData.swapChain, nullptr);
    _swapChainData.swapChain = VK_NULL_HANDLE;
}
swapchain::~swapchain()
{
}
