//
//  depth_image.cpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 2/17/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#include "depth_image.h"

using namespace vk;

void depth_image::create(uint32_t width, uint32_t height)
{
    assert(!_created);
    
    VkFormat depthFormat = _device->find_depth_format( );
    
    _width = width;
    _height = height;
    
    create_image( width, height, depthFormat,
                VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    
    create_image_view( _image, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, _imageView);
    
    change_image_layout(_device->_commandPool, _device->_graphics_queue, _image, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED,
                      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    
    _created = true;
}

void depth_image::destroy()
{
    if(_created)
    {
        
        vkDestroyImageView(_device->_logical_device, _imageView, nullptr);
        vkDestroyImage(_device->_logical_device, _image, nullptr);
        vkFreeMemory(_device->_logical_device, _imageMemory, nullptr);
        _created = false;
        _image = VK_NULL_HANDLE;
        _imageMemory = VK_NULL_HANDLE;
        _imageView = VK_NULL_HANDLE;
    }
}

VkAttachmentDescription depth_image::get_depth_attachment()
{
    VkAttachmentDescription depthAttachment = {};
    
    depthAttachment.flags = 0;
    depthAttachment.format =_device->find_depth_format();
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    return depthAttachment;
}

