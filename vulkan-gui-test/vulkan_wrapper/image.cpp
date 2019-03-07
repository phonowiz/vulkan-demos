//
//  image.cpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 2/15/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#include "image.h"

using namespace vk;

void Image::createImage( uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
                          VkImageUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags)
{
    
    VkImageCreateInfo imageCreateInfo = {};
    
    _width = width;
    _height = height;
    
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.pNext  = nullptr;
    imageCreateInfo.flags = 0;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = format;
    imageCreateInfo.extent.width = width;
    imageCreateInfo.extent.height = height;
    imageCreateInfo.extent.depth = 1;
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.tiling = tiling;
    imageCreateInfo.usage = usageFlags;
    //the following assignment depends on this assumption:
    assert(_device->_presentQueue == _device->_graphicsQueue);
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.queueFamilyIndexCount = 0;
    imageCreateInfo.pQueueFamilyIndices = nullptr;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
    
    VkResult result = vkCreateImage(_device->_device, &imageCreateInfo, nullptr, &_image);
    ASSERT_VULKAN(result);
    VkMemoryRequirements memoryRequirements;
    vkGetImageMemoryRequirements(_device->_device, _image, &memoryRequirements);
    
    VkMemoryAllocateInfo memoryAllocateInfo;
    memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocateInfo.pNext = nullptr;
    memoryAllocateInfo.allocationSize = memoryRequirements.size;
    memoryAllocateInfo.memoryTypeIndex = findMemoryTypeIndex(_device->_physicalDevice, memoryRequirements.memoryTypeBits,
                                                             propertyFlags);
    result = vkAllocateMemory(_device->_device, &memoryAllocateInfo, nullptr, &_imageMemory);
    
    ASSERT_VULKAN(result);
    
    vkBindImageMemory(_device->_device, _image, _imageMemory, 0);
    
}

void Image::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageView &imageView)
{
    
    VkImageViewCreateInfo imageViewCreateInfo;
    
    imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCreateInfo.pNext = nullptr;
    imageViewCreateInfo.flags = 0;
    imageViewCreateInfo.image = image;
    imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCreateInfo.format = format;
    imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.subresourceRange.aspectMask = aspectFlags;
    imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
    imageViewCreateInfo.subresourceRange.levelCount = 1;
    imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    imageViewCreateInfo.subresourceRange.layerCount = 1;
    
    VkResult result = vkCreateImageView(_device->_device, &imageViewCreateInfo, nullptr, &imageView);
    ASSERT_VULKAN(result);
}

void Image::changeImageLayout(VkCommandPool commandPool, VkQueue queue, VkImage image,
                              VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
{
    VkCommandBuffer commandBuffer = _device->startSingleTimeCommandBuffer( commandPool);
    
    VkImageMemoryBarrier imageMemoryBarrier;
    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.pNext = nullptr;
    if( oldLayout == VK_IMAGE_LAYOUT_PREINITIALIZED &&
       newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    }
    else if( oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
            newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    }
    else if( oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
            newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        imageMemoryBarrier.srcAccessMask = 0;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }
    else
    {
        assert(0 && "transition not yet supported");
    }
    imageMemoryBarrier.oldLayout = oldLayout;
    imageMemoryBarrier.newLayout = newLayout;
    imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.image = image;
    if(newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        
        if( isStencilFormat(format))
        {
            imageMemoryBarrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    }
    else
    {
        imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }
    
    imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
    imageMemoryBarrier.subresourceRange.levelCount = 1;
    imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
    imageMemoryBarrier.subresourceRange.layerCount = 1;
    
    vkCmdPipelineBarrier(commandBuffer,
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, //operations in this pipeline stage should occur before the barrier
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, //operations in this pipeline stage will wait on the barrier
                         0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
    
    
    _device->endSingleTimeCommandBuffer( queue, commandPool, commandBuffer);
    
    
}
bool Image::isStencilFormat(VkFormat format)
{
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void Image::writeBufferToImage(VkCommandPool commandPool, VkQueue queue, VkBuffer buffer)
{
    
    VkCommandBuffer commandBuffer = _device->startSingleTimeCommandBuffer( commandPool);
    
    
    VkBufferImageCopy bufferImageCopy;
    
    bufferImageCopy.bufferOffset = 0;
    bufferImageCopy.bufferRowLength = 0;
    bufferImageCopy.bufferImageHeight = 0;
    bufferImageCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    bufferImageCopy.imageSubresource.mipLevel = 0;
    bufferImageCopy.imageSubresource.baseArrayLayer = 0;
    bufferImageCopy.imageSubresource.layerCount = 1;
    bufferImageCopy.imageOffset = { 0, 0, 0};
    bufferImageCopy.imageExtent = { static_cast<uint32_t>(getWidth()),
        static_cast<uint32_t>(getHeight()),
        1};
    
    vkCmdCopyBufferToImage(commandBuffer, buffer, _image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bufferImageCopy);
    
    
    _device->endSingleTimeCommandBuffer(queue, commandPool, commandBuffer);
}



