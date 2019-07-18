//
//  color_image.cpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 2/28/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#include "swapchain_image_set.h"



using namespace vk;

void swapchain_image_set::create_image_set()
{
    vkGetSwapchainImagesKHR(_device->_logical_device, _swapchain, &_image_count, nullptr);
    vkGetSwapchainImagesKHR(_device->_logical_device, _swapchain, &_image_count, _images.data());
}

void swapchain_image_set::create_image_views(VkFormat format)
{
    for(int i = 0; i < _image_count; ++i)
    {
        create_image_view(_images[i], format, VK_IMAGE_ASPECT_COLOR_BIT, _imageViews[i]);
    }
}

void swapchain_image_set::destroy()
{
    for (int i = 0; i < _image_count; ++i)
    {
        vkDestroyImageView(_device->_logical_device, _imageViews[i], nullptr);
        _imageViews[i] = VK_NULL_HANDLE;
    }
}
