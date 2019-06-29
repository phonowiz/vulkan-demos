//
//  image.cpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 2/15/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#include "image.h"

using namespace vk;

void image::create_image( uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
                          VkImageUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags)
{
    
    VkImageCreateInfo image_create_info = {};
    
    _width = width;
    _height = height;
    
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.pNext  = nullptr;
    image_create_info.flags = 0;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = format;
    image_create_info.extent.width = width;
    image_create_info.extent.height = height;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.tiling = tiling;
    image_create_info.usage = usageFlags;
    //the following assignment depends on this assumption:
    assert(_device->_presentQueue == _device->_graphics_queue);
    image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_create_info.queueFamilyIndexCount = 0;
    image_create_info.pQueueFamilyIndices = nullptr;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
    
    VkResult result = vkCreateImage(_device->_logical_device, &image_create_info, nullptr, &_image);
    ASSERT_VULKAN(result);
    VkMemoryRequirements memoryRequirements;
    vkGetImageMemoryRequirements(_device->_logical_device, _image, &memoryRequirements);
    
    VkMemoryAllocateInfo memoryAllocateInfo;
    memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocateInfo.pNext = nullptr;
    memoryAllocateInfo.allocationSize = memoryRequirements.size;
    memoryAllocateInfo.memoryTypeIndex = find_memory_type_index(_device->_physical_device, memoryRequirements.memoryTypeBits,
                                                             propertyFlags);
    result = vkAllocateMemory(_device->_logical_device, &memoryAllocateInfo, nullptr, &_imageMemory);
    
    ASSERT_VULKAN(result);
    
    vkBindImageMemory(_device->_logical_device, _image, _imageMemory, 0);
    
}

void image::create_image_view(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageView &imageView)
{
    
    VkImageViewCreateInfo image_view_create_info;
    
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
    image_view_create_info.subresourceRange.aspectMask = aspectFlags;
    image_view_create_info.subresourceRange.baseMipLevel = 0;
    image_view_create_info.subresourceRange.levelCount = 1;
    image_view_create_info.subresourceRange.baseArrayLayer = 0;
    image_view_create_info.subresourceRange.layerCount = 1;
    
    VkResult result = vkCreateImageView(_device->_logical_device, &image_view_create_info, nullptr, &imageView);
    ASSERT_VULKAN(result);
}

void image::change_image_layout(VkCommandPool commandPool, VkQueue queue, VkImage image,
                              VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
{
    VkCommandBuffer commandBuffer = _device->start_single_time_command_buffer( commandPool);
    
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
        
        if( is_stencil_format(format))
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
    
    
    _device->end_single_time_command_buffer( queue, commandPool, commandBuffer);
    
    
}
bool image::is_stencil_format(VkFormat format)
{
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void image::write_buffer_to_image(VkCommandPool commandPool, VkQueue queue, VkBuffer buffer)
{
    
    VkCommandBuffer commandBuffer = _device->start_single_time_command_buffer( commandPool);
    
    
    VkBufferImageCopy bufferImageCopy;
    
    bufferImageCopy.bufferOffset = 0;
    bufferImageCopy.bufferRowLength = 0;
    bufferImageCopy.bufferImageHeight = 0;
    bufferImageCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    bufferImageCopy.imageSubresource.mipLevel = 0;
    bufferImageCopy.imageSubresource.baseArrayLayer = 0;
    bufferImageCopy.imageSubresource.layerCount = 1;
    bufferImageCopy.imageOffset = { 0, 0, 0};
    bufferImageCopy.imageExtent = { static_cast<uint32_t>(get_width()),
        static_cast<uint32_t>(get_height()),
        1};
    
    vkCmdCopyBufferToImage(commandBuffer, buffer, _image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bufferImageCopy);
    
    
    _device->end_single_time_command_buffer(queue, commandPool, commandBuffer);
}



