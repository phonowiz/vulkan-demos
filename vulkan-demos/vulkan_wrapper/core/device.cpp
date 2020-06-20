//
//  physical_device.cpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 2/7/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#include "device.h"

#include "EASTL/fixed_vector.h"
#include "EASTL/array.h"
#include <set>
#include <string>
#include <iostream>
#include <fstream>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "device.h"
#include "EAAssert/eaassert.h"

#if __APPLE__ && DEBUG
#include <MoltenVK/vk_mvk_moltenvk.h>
#include <dlfcn.h>

#endif

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>


using namespace vk;


//this function is meant to be private and not accessible to anybody outside of this file
VKAPI_ATTR VkBool32 VKAPI_CALL debug_report_callback(
VkDebugReportFlagsEXT       flags,
VkDebugReportObjectTypeEXT  objectType,
uint64_t                    object,
size_t                      location,
int32_t                     messageCode,
const char*                 pLayerPrefix,
const char*                 pMessage,
void*                       pUserData)
{
    if( (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT))
    {
        std::cout << "DEBUG REPORT ERROR: " << std::endl;
    }
    else if( (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT))
    {
        std::cout << "PERFORMANCE WARNING: " << std::endl;
    }
    else if( (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT))
    {
        std::cout << "DEBUG REPORT WARNING: "<< std::endl;
    }
    
    std::cout << "   layer prefix: " << pLayerPrefix << std::endl;
    std::cout << "   msg: " << pMessage << std::endl;
    
    if((flags & VK_DEBUG_REPORT_ERROR_BIT_EXT))
        EA_DEBUG_BREAK();
    
    return VK_FALSE;
}

device::device()
{
    create_instance();
#if __APPLE__ && DEBUG
    //NOTE: this code is here in case we decide to call moltenvk driver directly instead of using lunarg laoder
    //MVKConfiguration config {};
    //size_t config_size = sizeof(MVKConfiguration);
    //vkGetMoltenVKConfigurationMVK( _instance, &config, &config_size );
    
    //config.performanceLoggingFrameCount = 1;
    //config.performanceTracking = 1;
#endif
}

void device::create_logical_device( VkSurfaceKHR surface)
{
    pick_physical_device(surface);
    _queue_family_indices = find_queue_families(_physical_device, surface);

    eastl::fixed_vector<VkDeviceQueueCreateInfo,20, true> queue_create_infos {};
    std::set<uint32_t> unique_queue_families = {_queue_family_indices.graphics_family.value(), _queue_family_indices.present_family.value()};

    float queue_priority = 1.0f;
    for (uint32_t queueFamily : unique_queue_families) {
        VkDeviceQueueCreateInfo queue_create_info = {};
        queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info.queueFamilyIndex = queueFamily;
        queue_create_info.queueCount = 1;
        queue_create_info.pQueuePriorities = &queue_priority;
        queue_create_infos.push_back(queue_create_info);
    }

    VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT features_ext = {};
    
    VkPhysicalDeviceFeatures device_features = {};
    
    device_features.samplerAnisotropy = VK_TRUE;
    device_features.shaderStorageImageWriteWithoutFormat = VK_TRUE;
    device_features.fragmentStoresAndAtomics = VK_TRUE;
    device_features.independentBlend = VK_TRUE;
    
    VkPhysicalDeviceFeatures2 device_features_2 = {};
    
    features_ext.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_INTERLOCK_FEATURES_EXT;
    features_ext.fragmentShaderPixelInterlock = VK_FALSE;
//    device_features_2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_INTERLOCK_FEATURES_EXT;
//    device_features_2.pNext = &features_ext;
    
    device_features_2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    device_features_2.pNext = &features_ext;
    device_features_2.features = device_features;
    

    



#if !defined(__APPLE__)
    //not supported by mac os
    deviceFeatures.depthClamp = VK_TRUE;
    deviceFeatures.depthBounds = VK_TRUE;
#endif
    VkDeviceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.pNext = &device_features_2;
    create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
    create_info.pQueueCreateInfos = queue_create_infos.data();

    create_info.pEnabledFeatures = nullptr;

    create_info.enabledExtensionCount = static_cast<uint32_t>(device::device_extensions.size());
    create_info.ppEnabledExtensionNames = device::device_extensions.data();

    if (device::enable_validation_layers)
    {
    #ifndef __APPLE__
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    #endif
    }
    else
    {
        create_info.enabledLayerCount = 0;
    }

    if (vkCreateDevice(_physical_device, &create_info, nullptr, &_logical_device) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }
    
    vkGetDeviceQueue(_logical_device, _queue_family_indices.graphics_family.value(), 0, &_graphics_queue);
    vkGetDeviceQueue(_logical_device, _queue_family_indices.present_family.value(), 0, &_present_queue);
    vkGetDeviceQueue(_logical_device, _queue_family_indices.compute_family.value(), 0, &_compute_queue);
    
    create_command_pool(_queue_family_indices.graphics_family.value(), &_graphics_command_pool);
    
    _present_command_pool = _graphics_command_pool;
    if(_queue_family_indices.graphics_family != _queue_family_indices.present_family)
    {
        create_command_pool(_queue_family_indices.present_family.value(), &_present_command_pool);
    }
    _compute_command_pool = _present_command_pool;
    
    if(_queue_family_indices.graphics_family == _queue_family_indices.compute_family)
    {
        _compute_command_pool = _graphics_command_pool;
    }
    else if( _queue_family_indices.compute_family != _queue_family_indices.present_family)
    {
        create_command_pool(_queue_family_indices.compute_family.value(), &_compute_command_pool);
    }
}

device::queue_family_indices device::find_queue_families( VkPhysicalDevice device, VkSurfaceKHR surface) {
    device::queue_family_indices indices;
    
    uint32_t queue_family_count = 0;
    
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);
    static const uint32_t MAX_QUEUE_FAMILIES = 200;
    EA_ASSERT_MSG(MAX_QUEUE_FAMILIES > queue_family_count, "There are more queue families than we can handle");
    eastl::array<VkQueueFamilyProperties, MAX_QUEUE_FAMILIES> queue_families;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());
    
    int i = 0;
    for (const auto& queue_family : queue_families) {
        
        if (queue_family.queueCount > 0 && queue_family.queueFlags & ( VK_QUEUE_COMPUTE_BIT))
        {
            indices.compute_family = i;
        }
        if (queue_family.queueCount > 0 && queue_family.queueFlags & (VK_QUEUE_GRAPHICS_BIT ))
        {
            indices.graphics_family = i;
        }
        
        VkBool32 present_support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_support);
        
        if (queue_family.queueCount > 0 && present_support) {
            indices.present_family = i;
        }
        
        if (indices.is_complete()) {
            break;
        }
        
        i++;
    }
    
    return indices;
}

bool device::check_device_extension_support(VkPhysicalDevice device)
{
    
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    
    eastl::fixed_vector<VkExtensionProperties, 20, true> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
    
    std::set<std::string> requiredExtensions(device_extensions.begin(), device_extensions.end());
    
    for (const VkExtensionProperties& extension : availableExtensions)
    {
        requiredExtensions.erase(&extension.extensionName[0]);
    }
    
    return requiredExtensions.empty();
}

void device::query_swapchain_support( VkPhysicalDevice device, VkSurfaceKHR surface, device::swapchain_support_details& details)
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

void device::print_stats()
{

    std::cout << "Name:                     " << _properties.deviceName << std::endl;
    uint32_t apiVer = _properties.apiVersion;
    std::cout << "API Version:              " << VK_VERSION_MAJOR(apiVer) << "." << VK_VERSION_MINOR(apiVer) << "." << VK_VERSION_PATCH(apiVer) << std::endl;
    std::cout << "Driver Version:           " << _properties.driverVersion << std::endl;
    std::cout << "Vendor ID:                " << _properties.vendorID << std::endl;
    std::cout << "Device ID:                " << _properties.deviceID << std::endl;
    std::cout << "Device Type:              " << _properties.deviceType << std::endl;
    std::cout << "discreteQueuePriorities:  " << _properties.limits.discreteQueuePriorities << std::endl;
    
    VkPhysicalDeviceFeatures features {};
    
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
void device::create_instance()
{
    VkApplicationInfo app_info;
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pNext = nullptr;
    app_info.pApplicationName = "Rafael's Demo";
    app_info.applicationVersion = VK_MAKE_VERSION(0, 0, 0);
    app_info.pEngineName = "Super Vulkan Engine Turbo Mega";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;
    
    const eastl::array<const char*, 1> validation_layers = {
#if DEBUG || defined(_DEBUG)
        "VK_LAYER_KHRONOS_validation"
#endif
    };
    
    uint32_t glfw_extensions_count = 0;

    auto glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extensions_count);
    
    eastl::array<const char*, 10> all_required_extensions {};
    //all_required_extensions[0] = "VK_EXT_debug_report";
    
    EA_ASSERT(all_required_extensions.size() > (glfw_extensions_count + 1));
    int i = 0;
    for( ; i < glfw_extensions_count; ++i)
    {
        all_required_extensions[i] = glfw_extensions[i];
    }
    //NOTE: Keep in mind that the order in which you load extensions matters, careful in loading something too early
    all_required_extensions[i] = "VK_EXT_debug_report";
    all_required_extensions[++i] = "VK_KHR_get_physical_device_properties2";

    VkInstanceCreateInfo instanceInfo;
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pNext = nullptr;
    instanceInfo.flags = 0;
    instanceInfo.pApplicationInfo = &app_info;
    instanceInfo.enabledLayerCount = (uint32_t)validation_layers.size();
    instanceInfo.ppEnabledLayerNames = validation_layers.data();
    instanceInfo.enabledExtensionCount = i + 1;
    instanceInfo.ppEnabledExtensionNames = all_required_extensions.data();
    
    uint32_t instance_layer_count {};
    eastl::array<VkLayerProperties, 200> layer_properties {};
    VkResult result = vkEnumerateInstanceLayerProperties(&instance_layer_count, nullptr);
    ASSERT_VULKAN(result);
    assert(instance_layer_count < layer_properties.size());
    std::cout << instance_layer_count << " instance layers have been found\n";
    if (instance_layer_count > 0) {
        
        result = vkEnumerateInstanceLayerProperties(&instance_layer_count, layer_properties.data());
        for (int i = 0; i < instance_layer_count; ++i) {
            std::cout << layer_properties[i].layerName << "\n";
        }
    }
    
    uint32_t instance_extension_count = 0;
    eastl::array<VkExtensionProperties, 100> vk_props = {};
    vkEnumerateInstanceExtensionProperties(NULL, &instance_extension_count, NULL);
    vkEnumerateInstanceExtensionProperties(NULL, &instance_extension_count, vk_props.data());
    std::cout << std::endl;
    std::cout << instance_extension_count << " instance extensions have been found " << std::endl;
    for (uint32_t i = 0; i < instance_extension_count; i++) {
        std::cout << vk_props[i].extensionName << std::endl;
    }
    
    result = vkCreateInstance(&instanceInfo, nullptr, &_instance);
    
    ASSERT_VULKAN(result);
    
    VkDebugReportCallbackCreateInfoEXT callback_create_info {};
    callback_create_info.sType       = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
    callback_create_info.pNext       = nullptr;
    callback_create_info.flags       = VK_DEBUG_REPORT_ERROR_BIT_EXT |
                                     VK_DEBUG_REPORT_WARNING_BIT_EXT |
                                     VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
    callback_create_info.pfnCallback = &debug_report_callback;
    callback_create_info.pUserData   = nullptr;

    
    PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT =
        reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>
               (vkGetInstanceProcAddr(_instance, "vkCreateDebugReportCallbackEXT"));

    assert(vkCreateDebugReportCallbackEXT != nullptr);
    //Register the callback
    result = vkCreateDebugReportCallbackEXT(_instance, &callback_create_info, nullptr, &_callback);
    ASSERT_VULKAN(result);
}

//vulkan renderer
void device::print_instance_layers()
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
void device::print_instance_extensions()
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

bool device::is_format_supported( VkFormat format,
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

VkFormat device::find_support_format(const eastl::fixed_vector<VkFormat, 20, true>& formats,
                                    VkImageTiling tiling, VkFormatFeatureFlags featureFlags)
{
    for( VkFormat format : formats)
    {
        if(is_format_supported( format, tiling, featureFlags))
        {
            return format;
        }
    }
    assert(0 && "no supported format found!");
    return VK_FORMAT_UNDEFINED;
}


//vulkan command
VkCommandBuffer device::start_single_time_command_buffer( VkCommandPool command_pool)
{
    VkCommandBufferAllocateInfo command_buffer_allocate_info = {};
    command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.pNext = nullptr;
    command_buffer_allocate_info.commandPool = command_pool;
    command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_allocate_info.commandBufferCount = 1;
    
    VkCommandBuffer command_buffer = {};
    VkResult result = vkAllocateCommandBuffers(_logical_device, &command_buffer_allocate_info, &command_buffer);
    ASSERT_VULKAN(result);
    
    VkCommandBufferBeginInfo command_buffer_begin_info {};
    command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_begin_info.pNext = nullptr;
    command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    command_buffer_begin_info.pInheritanceInfo = nullptr;
    
    result = vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info);
    ASSERT_VULKAN(result);
    
    return command_buffer;
    
}

void device::create_command_pool(uint32_t queue_index, VkCommandPool* pool)
{
    VkCommandPoolCreateInfo command_pool_create_info;
    command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_create_info.pNext = nullptr;
    command_pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    command_pool_create_info.queueFamilyIndex = queue_index;
    
    VkResult result = vkCreateCommandPool(_logical_device, &command_pool_create_info, nullptr, pool);
    ASSERT_VULKAN(result);
}


//vulkan command
void device::end_single_time_command_buffer( VkQueue queue, VkCommandPool commandPool, VkCommandBuffer commandBuffer)
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

VkFormat device::find_depth_format()
{
    //the order here matters as the "findsupportedformat" function returns the first one that is supported
    //here we have the 32 bits depth with 8 bits of stencil
    eastl::fixed_vector<VkFormat, 20, true> possibleFormats = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT
    };
    
    return find_support_format( possibleFormats, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}


//device function
void device::pick_physical_device(VkSurfaceKHR surface)
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(_instance, &deviceCount, nullptr);
    assert(deviceCount != 0);
    
    
    eastl::fixed_vector<VkPhysicalDevice, 20, true> devices(deviceCount);
    vkEnumeratePhysicalDevices(_instance, &deviceCount, devices.data());
    
    for (const VkPhysicalDevice& device : devices)
    {
        vkGetPhysicalDeviceProperties(device, &_properties);
        
        if (is_device_suitable(device, surface))
        {
            
            if(_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
            {

                _physical_device = device;
                continue;
            }
            else if( _properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            {
                _physical_device = device;
                break;
            }
            else
            {
                _physical_device = device;
                continue;
            }
            
        }
    }
}

bool device::is_device_suitable( VkPhysicalDevice device, VkSurfaceKHR surface)
{
    device::queue_family_indices indices = find_queue_families(device, surface);
    
    bool extensionsSupported = check_device_extension_support(device);
    
    bool swapChainAdequate = false;
    if (extensionsSupported) {
        device::swapchain_support_details swapChainSupport;
        query_swapchain_support(device, surface, swapChainSupport);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }
    
    return indices.is_complete() && extensionsSupported && swapChainAdequate;
}

void device::wait_for_all_operations_to_finish()
{
    vkDeviceWaitIdle(_logical_device);
}

void device::destroy()
{
    PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT =
        reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>
            (vkGetInstanceProcAddr(_instance, "vkDestroyDebugReportCallbackEXT"));
    
    vkDestroyDebugReportCallbackEXT(_instance, _callback, nullptr);
    vkDestroyCommandPool(_logical_device, _graphics_command_pool, nullptr);
    
    vkDestroyDevice(_logical_device, nullptr);
    vkDestroyInstance(_instance, nullptr);
    
    _logical_device = VK_NULL_HANDLE;
    _instance = VK_NULL_HANDLE;
}


void device::copy_buffer( VkCommandPool commandPool, VkQueue queue, VkBuffer src, VkBuffer dest, VkDeviceSize size)
{
    
    VkCommandBuffer commandBuffer = start_single_time_command_buffer( commandPool);
    
    VkBufferCopy bufferCopy = {};
    bufferCopy.dstOffset = 0;
    bufferCopy.srcOffset = 0;
    bufferCopy.srcOffset = 0;
    bufferCopy.size = size;
    vkCmdCopyBuffer(commandBuffer, src, dest, 1, &bufferCopy);
    
    end_single_time_command_buffer( queue, commandPool, commandBuffer);
    
}

device::~device()
{
    
}

