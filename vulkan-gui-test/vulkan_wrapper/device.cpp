//
//  physical_device.cpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 2/7/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#include "device.h"

#include <vector>
#include <array>
#include <set>
#include <string>
#include <iostream>
#include <fstream>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "device.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

using namespace vk;

device::device()
{
    createInstance();
}

void device::createLogicalDevice( VkSurfaceKHR surface)
{
    pickPhysicalDevice(surface);
    device::QueueFamilyIndices indices = findQueueFamilies(_physical_device, surface);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures = {};
    deviceFeatures.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();

    createInfo.pEnabledFeatures = &deviceFeatures;

    createInfo.enabledExtensionCount = static_cast<uint32_t>(device::deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = device::deviceExtensions.data();

    if (device::enableValidationLayers)
    {
    #ifndef __APPLE__
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    #endif
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(_physical_device, &createInfo, nullptr, &_logical_device) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }

    vkGetDeviceQueue(_logical_device, indices.graphicsFamily.value(), 0, &_graphics_queue);
    vkGetDeviceQueue(_logical_device, indices.presentFamily.value(), 0, &_presentQueue);
    
    createCommnadPool(0);
}

device::QueueFamilyIndices device::findQueueFamilies( VkPhysicalDevice device, VkSurfaceKHR surface) {
    device::QueueFamilyIndices indices;
    
    uint32_t queueFamilyCount = 0;
    
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    static const uint32_t MAX_QUEUE_FAMILIES = 200;
    assert(MAX_QUEUE_FAMILIES > queueFamilyCount);
    std::array<VkQueueFamilyProperties, MAX_QUEUE_FAMILIES> queueFamilies;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
    
    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }
        
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        
        if (queueFamily.queueCount > 0 && presentSupport) {
            indices.presentFamily = i;
        }
        
        if (indices.isComplete()) {
            break;
        }
        
        i++;
    }
    
    return indices;
}

bool device::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
    
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
    
    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
    
    for (const VkExtensionProperties& extension : availableExtensions)
    {
        requiredExtensions.erase(&extension.extensionName[0]);
    }
    
    return requiredExtensions.empty();
}

void device::querySwapChainSupport( VkPhysicalDevice device, VkSurfaceKHR surface, device::SwapChainSupportDetails& details)
{
    
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
    
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
    
    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }
    
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
    
    if (presentModeCount != 0)
    {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }
}

void device::printStats()
{
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(_physical_device, &properties);
    
    std::cout << "Name:                     " << properties.deviceName << std::endl;
    uint32_t apiVer = properties.apiVersion;
    std::cout << "API Version:              " << VK_VERSION_MAJOR(apiVer) << "." << VK_VERSION_MINOR(apiVer) << "." << VK_VERSION_PATCH(apiVer) << std::endl;
    std::cout << "Driver Version:           " << properties.driverVersion << std::endl;
    std::cout << "Vendor ID:                " << properties.vendorID << std::endl;
    std::cout << "Device ID:                " << properties.deviceID << std::endl;
    std::cout << "Device Type:              " << properties.deviceType << std::endl;
    std::cout << "discreteQueuePriorities:  " << properties.limits.discreteQueuePriorities << std::endl;
    
    VkPhysicalDeviceFeatures features;
    
    vkGetPhysicalDeviceFeatures(_physical_device, &features);
    std::cout << "Geometry Shader:          " << features.geometryShader << std::endl;
    
    VkPhysicalDeviceMemoryProperties memProp;
    vkGetPhysicalDeviceMemoryProperties(_physical_device, &memProp);
    
    uint32_t amountOfQueueFamilies = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(_physical_device, &amountOfQueueFamilies, nullptr);
    VkQueueFamilyProperties *familyProperties = new VkQueueFamilyProperties[amountOfQueueFamilies];
    vkGetPhysicalDeviceQueueFamilyProperties(_physical_device, &amountOfQueueFamilies, familyProperties);
    
    std::cout << "Amount of Queue Families: " << amountOfQueueFamilies << std::endl;
    
    for (int i = 0; i < amountOfQueueFamilies; i++) {
        std::cout << std::endl;
        std::cout << "Queue Family #" << i << std::endl;
        std::cout << "VK_QUEUE_GRAPHICS_BIT       " << ((familyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) << std::endl;
        std::cout << "VK_QUEUE_COMPUTE_BIT        " << ((familyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) != 0) << std::endl;
        std::cout << "VK_QUEUE_TRANSFER_BIT       " << ((familyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT) != 0) << std::endl;
        std::cout << "VK_QUEUE_SPARSE_BINDING_BIT " << ((familyProperties[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) != 0) << std::endl;
        std::cout << "Queue Count: " << familyProperties[i].queueCount << std::endl;
        std::cout << "Timestamp Valid Bits: " << familyProperties[i].timestampValidBits << std::endl;
        uint32_t width = familyProperties[i].minImageTransferGranularity.width;
        uint32_t height = familyProperties[i].minImageTransferGranularity.height;
        uint32_t depth = familyProperties[i].minImageTransferGranularity.depth;
        std::cout << "Min Image Transfer Granularity: " << width << ", " << height << ", " << depth << std::endl;
    }
    
    std::cout << std::endl;
    delete[] familyProperties;
}
void device::createInstance()
{
    VkApplicationInfo appInfo;
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pNext = nullptr;
    appInfo.pApplicationName = "Vulkan Tutorial";
    appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 0);
    appInfo.pEngineName = "Super Vulkan Engine Turbo Mega";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;
    
    const std::vector<const char*> validationLayers = {
#ifndef __APPLE__
        "VK_LAYER_LUNARG_standard_validation"
#endif
    };
    
    uint32_t amountOfGlfwExtensions = 0;
    auto glfwExtensions = glfwGetRequiredInstanceExtensions(&amountOfGlfwExtensions);
    
    
    
    VkInstanceCreateInfo instanceInfo;
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pNext = nullptr;
    instanceInfo.flags = 0;
    instanceInfo.pApplicationInfo = &appInfo;
    instanceInfo.enabledLayerCount = (uint32_t)validationLayers.size();
    instanceInfo.ppEnabledLayerNames = validationLayers.data();
    instanceInfo.enabledExtensionCount = amountOfGlfwExtensions;
    instanceInfo.ppEnabledExtensionNames = glfwExtensions;
    
    
    VkResult result = vkCreateInstance(&instanceInfo, nullptr, &_instance);
    
    ASSERT_VULKAN(result);
}

//vulkan renderer
void device::printInstanceLayers()
{
    uint32_t amountOfLayers = 0;
    vkEnumerateInstanceLayerProperties(&amountOfLayers, nullptr);
    VkLayerProperties *layers = new VkLayerProperties[amountOfLayers];
    vkEnumerateInstanceLayerProperties(&amountOfLayers, layers);
    
    std::cout << "Amount of Instance Layers: " << amountOfLayers << std::endl;
    for (int i = 0; i < amountOfLayers; i++) {
        std::cout << std::endl;
        std::cout << "Name:         " << layers[i].layerName << std::endl;
        std::cout << "Spec Version: " << layers[i].specVersion << std::endl;
        std::cout << "Impl Version: " << layers[i].implementationVersion << std::endl;
        std::cout << "Description:  " << layers[i].description << std::endl;
    }
    delete[] layers;
}

//vulkan renderer
void device::printInstanceExtensions()
{
    uint32_t amountOfExtensions = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &amountOfExtensions, nullptr);
    VkExtensionProperties *extensions = new VkExtensionProperties[amountOfExtensions];
    vkEnumerateInstanceExtensionProperties(nullptr, &amountOfExtensions, extensions);
    
    std::cout << std::endl;
    std::cout << "Amount of Extensions: " << amountOfExtensions << std::endl;
    for (int i = 0; i < amountOfExtensions; i++) {
        std::cout << std::endl;
        std::cout << "Name: " << extensions[i].extensionName << std::endl;
        std::cout << "Spec Version: " << extensions[i].specVersion << std::endl;
    }
    
    delete[] extensions;
}

bool device::isFormatSupported( VkFormat format,
                                       VkImageTiling tiling, VkFormatFeatureFlags featureFlags)
{
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(_physical_device, format, &formatProperties);
    
    if(tiling == VK_IMAGE_TILING_LINEAR &&
       (formatProperties.linearTilingFeatures & featureFlags) == featureFlags)
    {
        return true;
    }
    else if( tiling == VK_IMAGE_TILING_OPTIMAL &&
            (formatProperties.optimalTilingFeatures & featureFlags) == featureFlags)
    {
        return true;
    }
    return false;
    
}

VkFormat device::findSupportedFormat(const std::vector<VkFormat>& formats,
                                    VkImageTiling tiling, VkFormatFeatureFlags featureFlags)
{
    for( VkFormat format : formats)
    {
        if(isFormatSupported( format, tiling, featureFlags))
        {
            return format;
        }
    }
    assert(0 && "no supported format found!");
    return VK_FORMAT_UNDEFINED;
}


//vulkan command
VkCommandBuffer device::startSingleTimeCommandBuffer( VkCommandPool commandPool)
{
    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.pNext = nullptr;
    commandBufferAllocateInfo.commandPool = commandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = 1;
    
    VkCommandBuffer commandBuffer = {};
    VkResult result = vkAllocateCommandBuffers(_logical_device, &commandBufferAllocateInfo, &commandBuffer);
    ASSERT_VULKAN(result);
    
    VkCommandBufferBeginInfo commandBufferBeginInfo;
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    commandBufferBeginInfo.pNext = nullptr;
    commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    commandBufferBeginInfo.pInheritanceInfo = nullptr;
    
    result = vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);
    ASSERT_VULKAN(result);
    
    return commandBuffer;
    
}

void device::createCommnadPool(uint32_t queueIndex)
{
    VkCommandPoolCreateInfo commandPoolCreateInfo;
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.pNext = nullptr;
    commandPoolCreateInfo.flags = 0;
    commandPoolCreateInfo.queueFamilyIndex = queueIndex; //TODO: FIND OUT WHAT QUEUE WE PUT HERE;
    
    VkResult result = vkCreateCommandPool(_logical_device, &commandPoolCreateInfo, nullptr, &_commandPool);
    ASSERT_VULKAN(result);
}


//vulkan command
void device::endSingleTimeCommandBuffer( VkQueue queue, VkCommandPool commandPool, VkCommandBuffer commandBuffer)
{
    VkResult result = vkEndCommandBuffer(commandBuffer);
    ASSERT_VULKAN(result);
    
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext  = nullptr;
    submitInfo.waitSemaphoreCount = 0;
    submitInfo.pWaitSemaphores = nullptr;
    submitInfo.pWaitDstStageMask = nullptr;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    submitInfo.signalSemaphoreCount = 0;
    submitInfo.pSignalSemaphores = nullptr;
    
    result = vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    ASSERT_VULKAN(result);
    
    vkQueueWaitIdle(queue);
    
    vkFreeCommandBuffers(_logical_device, commandPool, 1, &commandBuffer);
}

VkFormat device::findDepthFormat()
{
    //the order here matters as the "findsupportedformat" function returns the first one that is supported
    //here we have the 32 bits depth with 8 bits of stencil
    std::vector<VkFormat> possibleFormats = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT
    };
    
    return findSupportedFormat( possibleFormats, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}


//device function
void device::pickPhysicalDevice(VkSurfaceKHR surface)
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(_instance, &deviceCount, nullptr);
    assert(deviceCount != 0);
    
    
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(_instance, &deviceCount, devices.data());
    
    for (const VkPhysicalDevice& device : devices)
    {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(device, &properties);
        
        if (isDeviceSuitable(device, surface))
        {
            
            if(properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
            {
                
                _physical_device = device;
#ifdef __APPLE__
                //For my macbook pro, the integrated device runs much better than the discrete gpu,
                //I cannot explain why this happens.
                break;
#else
                continue;
#endif
            }
            else if( properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            {
                _physical_device = device;
#ifdef __APPLE__
                continue;
#else
                break;
#endif
            }
            else
            {
                _physical_device = device;
                continue;
            }
            
        }
    }
}

bool device::isDeviceSuitable( VkPhysicalDevice device, VkSurfaceKHR surface)
{
    device::QueueFamilyIndices indices = findQueueFamilies(device, surface);
    
    bool extensionsSupported = checkDeviceExtensionSupport(device);
    
    bool swapChainAdequate = false;
    if (extensionsSupported) {
        device::SwapChainSupportDetails swapChainSupport;
        querySwapChainSupport(device, surface, swapChainSupport);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }
    
    return indices.isComplete() && extensionsSupported && swapChainAdequate;
}

void device::waitForllOperationsToFinish()
{
    vkDeviceWaitIdle(_logical_device);
}

void device::destroy()
{
    vkDestroyCommandPool(_logical_device, _commandPool, nullptr);
    
    vkDestroyDevice(_logical_device, nullptr);
    vkDestroyInstance(_instance, nullptr);
    
    _logical_device = VK_NULL_HANDLE;
    _instance = VK_NULL_HANDLE;
}


void device::copyBuffer( VkCommandPool commandPool, VkQueue queue, VkBuffer src, VkBuffer dest, VkDeviceSize size)
{
    
    VkCommandBuffer commandBuffer = startSingleTimeCommandBuffer( commandPool);
    
    VkBufferCopy bufferCopy = {};
    bufferCopy.dstOffset = 0;
    bufferCopy.srcOffset = 0;
    bufferCopy.srcOffset = 0;
    bufferCopy.size = size;
    vkCmdCopyBuffer(commandBuffer, src, dest, 1, &bufferCopy);
    
    endSingleTimeCommandBuffer( queue, commandPool, commandBuffer);
    
}

device::~device()
{
    
}

