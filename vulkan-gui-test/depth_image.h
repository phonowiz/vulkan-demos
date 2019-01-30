//
//  depth_image.h
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 1/25/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#pragma once

#include "vulkan_utils.h"


class DepthImage
{
private:
    
    VkImage image = VK_NULL_HANDLE;
    
    VkDeviceMemory imageMemory = VK_NULL_HANDLE;
    VkImageView imageView = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    
    bool created = false;
    
public:
    DepthImage()
    {
        
    }
    
    
    void create(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue queue, uint32_t width, uint32_t height)
    {
        assert(!created);
        
        this->device = device;
        VkFormat depthFormat = findDepthFormat(physicalDevice);
        
        createImage(device, physicalDevice, width, height, depthFormat,
                    VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, imageMemory);
        
        createImageView(device, image, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, imageView);
        
        changeImageLayout(device, commandPool, queue, image, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
        
        created = true;
    }
    
    void destroy()
    {
        assert(created);

        vkDestroyImageView(device, imageView, nullptr);
        vkDestroyImage(device, image, nullptr);
        vkFreeMemory(device, imageMemory, nullptr);
        created = false;
        image = VK_NULL_HANDLE;
        imageMemory = VK_NULL_HANDLE;
        imageView = VK_NULL_HANDLE;
        device = VK_NULL_HANDLE;
    }
    
    VkImageView getImageView()
    {
        return imageView;
    }
    static VkFormat findDepthFormat(VkPhysicalDevice physicalDevice)
    {
        //the order here matters as the "findsupportedformat" function returns the first one that is supported
        //here we have the 32 bits depth with 8 bits of stencil
        std::vector<VkFormat> possibleFormats = {
                                                VK_FORMAT_D32_SFLOAT,
                                                VK_FORMAT_D32_SFLOAT_S8_UINT,
                                                VK_FORMAT_D24_UNORM_S8_UINT
                                                };

        return findSupportedFormat(physicalDevice, possibleFormats, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

    }
    
    static VkAttachmentDescription getDepthAttachment(VkPhysicalDevice physicalDevice)
    {
        VkAttachmentDescription depthAttachment = {};

        depthAttachment.flags = 0;
        depthAttachment.format =findDepthFormat(physicalDevice);
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        
        return depthAttachment;
    }
    
    static VkPipelineDepthStencilStateCreateInfo getDepthStencilStateCreateInfoOpaque()
    {
        VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = {};
        
        depthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencilStateCreateInfo.pNext = nullptr;
        depthStencilStateCreateInfo.flags = 0;
        depthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
        depthStencilStateCreateInfo.depthWriteEnable = VK_TRUE;
        depthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
        
        //depth bounds uses the min/max depth bounds below to see if the fragment is within the bounding box
        //we are currently not using this feature
        depthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
        depthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;
        depthStencilStateCreateInfo.front = {};
        depthStencilStateCreateInfo.back = {};
        depthStencilStateCreateInfo.minDepthBounds = 0.0f;
        depthStencilStateCreateInfo.maxDepthBounds = 1.0f;
        
        return depthStencilStateCreateInfo;
    }
};
