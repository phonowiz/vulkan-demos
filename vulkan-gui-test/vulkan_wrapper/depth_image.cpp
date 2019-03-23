//
//  depth_image.cpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 2/17/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#include "depth_image.h"

using namespace vk;

void DepthImage::create(uint32_t width, uint32_t height)
{
    assert(!_created);
    
    VkFormat depthFormat = _device->findDepthFormat( );
    
    _width = width;
    _height = height;
    
    createImage( width, height, depthFormat,
                VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    
    createImageView( _image, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, _imageView);
    
    changeImageLayout(_device->_commandPool, _device->_graphicsQueue, _image, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED,
                      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    
    _created = true;
}

void DepthImage::destroy()
{
    if(_created)
    {
        
        vkDestroyImageView(_device->_device, _imageView, nullptr);
        vkDestroyImage(_device->_device, _image, nullptr);
        vkFreeMemory(_device->_device, _imageMemory, nullptr);
        _created = false;
        _image = VK_NULL_HANDLE;
        _imageMemory = VK_NULL_HANDLE;
        _imageView = VK_NULL_HANDLE;
    }
}

VkAttachmentDescription DepthImage::getDepthAttachment()
{
    VkAttachmentDescription depthAttachment = {};
    
    depthAttachment.flags = 0;
    depthAttachment.format =_device->findDepthFormat();
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    return depthAttachment;
}

VkPipelineDepthStencilStateCreateInfo DepthImage::getDepthStencilStateCreateInfoOpaque()
{
    VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = {};
    
    depthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilStateCreateInfo.pNext = nullptr;
    depthStencilStateCreateInfo.flags = 0;
    depthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
    depthStencilStateCreateInfo.depthWriteEnable = VK_TRUE;
    depthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
    
    //todo:depth bounds uses the min/max depth bounds below to see if the fragment is within the bounding box
    //we are currently not using this feature
    depthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
    depthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;
    depthStencilStateCreateInfo.front = {};
    depthStencilStateCreateInfo.back = {};
    depthStencilStateCreateInfo.minDepthBounds = 0.0f;
    depthStencilStateCreateInfo.maxDepthBounds = 1.0f;
    
    return depthStencilStateCreateInfo;
}
