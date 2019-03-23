//
//  image.hpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 2/15/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#pragma once

#include "image.h"
#include "resource.h"
#include "device.h"

namespace vk
{
    class image : public resource
    {
    public:
        
        image(){}
        image( device* device){ _device = device; }
        void createImage( uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
                         VkImageUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags);
        
        void changeImageLayout(VkCommandPool commandPool, VkQueue queue, VkImage image,
                               VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
        
        void createImageView( VkImage image, VkFormat format,
                                    VkImageAspectFlags aspectFlags, VkImageView &imageView);
        
        bool isStencilFormat(VkFormat format);
        
        virtual ~image(){}
        
        void setDevice(device* device){ _device = device;}
        VkImage getImage()
        {
            return _image;
        }
        VkImageView getImageView()
        {
            //assert(loaded);
            return _imageView;
        }
        
        void writeBufferToImage(VkCommandPool commandPool, VkQueue queue, VkBuffer buffer);
        
//        Image(const Image &) = delete;
//        Image(Image &&) = delete;
//        Image& operator=(const Image &) = delete;
//        Image& operator=(Image &&) = delete;
        
        device* _device = nullptr;
        VkImage         _image = VK_NULL_HANDLE;
        VkDeviceMemory  _imageMemory = VK_NULL_HANDLE;
        VkImageView     _imageView = VK_NULL_HANDLE;
        
        virtual void create(uint32_t width, uint32_t height) = 0;
        
        int getWidth()
        {
            return _width;
        }
        
        int getHeight()
        {
            return _height;
        }
        
        uint32_t _width = 0;
        uint32_t _height = 0;
        
    };
}


