//
//  color_image.cpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 2/28/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#include "swapchain_image_set.h"



using namespace vk;

void SwapchainImageSet::createImageSet()
{
    vkGetSwapchainImagesKHR(_device->_device, _swapChain, &_imageCount, nullptr);
    vkGetSwapchainImagesKHR(_device->_device, _swapChain, &_imageCount, _images.data());
}

void SwapchainImageSet::createImageViews(VkFormat format)
{
    for(int i = 0; i < _imageCount; ++i)
    {
        createImageView(_images[i], format, VK_IMAGE_ASPECT_COLOR_BIT, _imageViews[i]);
    }
}

void SwapchainImageSet::destroy()
{
    for (int i = 0; i < _imageCount; ++i)
    {
        vkDestroyImageView(_device->_device, _imageViews[i], nullptr);
        _imageViews[i] = VK_NULL_HANDLE;
    }
}