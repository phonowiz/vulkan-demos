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
        void create_image( uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
                         VkImageUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags);
        
        void change_image_layout(VkCommandPool commandPool, VkQueue queue, VkImage image,
                               VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
        
        void create_image_view( VkImage image, VkFormat format,
                                    VkImageAspectFlags aspectFlags, VkImageView &imageView);
        
        bool is_stencil_format(VkFormat format);
        
        virtual ~image(){}
        
        void set_device(device* device){ _device = device;}
        VkImage get_image()
        {
            return _image;
        }
        VkImageView get_image_view()
        {
            //assert(loaded);
            return _imageView;
        }
        
        void write_buffer_to_image(VkCommandPool commandPool, VkQueue queue, VkBuffer buffer);
        
        
        device* _device = nullptr;
        VkImage         _image = VK_NULL_HANDLE;
        VkDeviceMemory  _imageMemory = VK_NULL_HANDLE;
        VkImageView     _imageView = VK_NULL_HANDLE;
        
        virtual void create(uint32_t width, uint32_t height) = 0;
        
        int get_width()
        {
            return _width;
        }
        
        int get_height()
        {
            return _height;
        }
        
        uint32_t _width = 0;
        uint32_t _height = 0;
        
    };
}


