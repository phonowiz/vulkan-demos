//
//  present_texture.cpp
//  vulkan-demos
//
//  Created by Rafael Sabino on 1/1/20.
//  Copyright © 2020 Rafael Sabino. All rights reserved.
//

#include "glfw_present_texture.h"
#include "glfw_swapchain.h"
#include <array>


using namespace vk;


VkPresentModeKHR glfw_present_texture::get_vk_present_mode()
{
    device::swapchain_support_details swapchain_support {};
    _device->query_swapchain_support( _device->_physical_device, _swapchain->get_vk_surface(), swapchain_support);
    
    //VkSurfaceFormatKHR surface_format = choose_swap_surface_format(swapchain_support.formats);
    VkPresentModeKHR present_mode = _swapchain->get_vk_swap_present_mode(swapchain_support.presentModes);
    
    return present_mode;
}

void glfw_present_texture::init()
{
    assert(_swapchain_image_index != INVALID);
    assert(_device != nullptr);
    assert(_swapchain->get_vk_surface() != VK_NULL_HANDLE);
    
    device::swapchain_support_details swapchain_support {};
    _device->query_swapchain_support( _device->_physical_device, _swapchain->get_vk_surface(), swapchain_support);
    
//    VkSurfaceFormatKHR surface_format = choose_swap_surface_format(swapchain_support.formats);
//    VkPresentModeKHR present_mode = choose_swap_present_mode(swapchain_support.presentModes);
    VkExtent2D extent = _swapchain->get_vk_swap_extent(swapchain_support.capabilities, *_window);
    
    _width = extent.width;
    _height = extent.height;
    _depth = 1;
    _filter = filter::NEAREST;
    _image_layout = image_layouts::PRESENT_KHR;
    _channels = 4;
    _mip_levels = 1;
    _format = formats::R8G8B8A8_UNSIGNED_NORMALIZED;
    
    std::array<VkImage, glfw_swapchain::NUM_SWAPCHAIN_IMAGES> images {};
    
    uint32_t image_count =0;
    vkGetSwapchainImagesKHR(_device->_logical_device, _swapchain->get_vk_swapchain(), &image_count, nullptr);
    vkGetSwapchainImagesKHR(_device->_logical_device, _swapchain->get_vk_swapchain(), &image_count, images.data());
    
    _format = static_cast<formats>(get_vk_surface_format().format);
    //VkSurfaceKHR  _surface = VK_NULL_HANDLE;
    _image = images[_swapchain_image_index];
    create_image_view(_image, static_cast<VkFormat>(_format), _image_view);
}
//
//VkSurfaceFormatKHR glfw_present_texture::choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& available_formats)
//{
//    if (available_formats.size() == 1 && available_formats[0].format == VK_FORMAT_UNDEFINED) {
//        return{VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
//    }
//
//    for (const auto& available_format : available_formats) {
//        if (available_format.format == VK_FORMAT_B8G8R8A8_UNORM && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
//            return available_format;
//        }
//    }
//
//    return available_formats[0];
//}

//VkPresentModeKHR glfw_present_texture::choose_swap_present_mode(const std::vector<VkPresentModeKHR>& available_present_modes)
//{
//    VkPresentModeKHR best_mode = VK_PRESENT_MODE_FIFO_KHR;
//
//    for (const auto& available_present_mode : available_present_modes) {
//        if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
//            return available_present_mode;
//        } else if (available_present_mode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
//            best_mode = available_present_mode;
//        }
//    }
//
//    return best_mode;
//}

//VkExtent2D glfw_present_texture::choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow& window)
//{
//    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
//    {
//        return capabilities.currentExtent;
//    }
//    else
//    {
//        int width, height;
//        glfwGetFramebufferSize(&window, &width, &height);
//
//        VkExtent2D actual_extent = {
//            static_cast<uint32_t>(width),
//            static_cast<uint32_t>(height)
//        };
//
//        actual_extent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actual_extent.width));
//        actual_extent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actual_extent.height));
//
//        return actual_extent;
//    }
//}

VkSurfaceFormatKHR glfw_present_texture::get_vk_surface_format()
{
    device::swapchain_support_details swapchain_support;
    _device->query_swapchain_support( _device->_physical_device, _swapchain->get_vk_surface(), swapchain_support);
    
    VkSurfaceFormatKHR surface_format = _swapchain->get_vk_swap_surface_format(swapchain_support.formats);
    return surface_format;
}

void glfw_present_texture::create_sampler()
{
    VkSamplerCreateInfo sampler_create_info {};
    
    sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_create_info.pNext = nullptr;
    sampler_create_info.flags = 0;
    sampler_create_info.magFilter = static_cast<VkFilter>(_filter);
    sampler_create_info.minFilter = static_cast<VkFilter>(_filter);
    sampler_create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_create_info.addressModeU  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_create_info.addressModeV =  VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_create_info.addressModeW =  VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_create_info.mipLodBias = 0.0f;
    sampler_create_info.anisotropyEnable = VK_TRUE;
    sampler_create_info.maxAnisotropy = 16;
    sampler_create_info.compareEnable = VK_FALSE;
    sampler_create_info.compareOp = VK_COMPARE_OP_ALWAYS;
    sampler_create_info.minLod = 0.0f;
    sampler_create_info.maxLod = static_cast<float>(_mip_levels);
    sampler_create_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_create_info.unnormalizedCoordinates = VK_FALSE;
    
    VkResult result = vkCreateSampler(_device->_logical_device, &sampler_create_info, nullptr, &_sampler);
    ASSERT_VULKAN(result);
}

void glfw_present_texture::create_image_view(VkImage image, VkFormat format, VkImageView& image_view)
{
    VkImageViewCreateInfo image_view_create_info {};
    
    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.pNext = nullptr;
    image_view_create_info.flags = 0;
    image_view_create_info.image = image;
    image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_create_info.format = format;
    image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.subresourceRange.aspectMask = _aspect_flag;
    image_view_create_info.subresourceRange.baseMipLevel = 0;
    image_view_create_info.subresourceRange.levelCount = _mip_levels;
    image_view_create_info.subresourceRange.baseArrayLayer = 0;
    image_view_create_info.subresourceRange.layerCount = _depth;
    assert(_depth == 1);
    VkResult result = vkCreateImageView(_device->_logical_device, &image_view_create_info, nullptr, &image_view);
    ASSERT_VULKAN(result);
}

