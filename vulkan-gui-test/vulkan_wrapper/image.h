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
        void create_image( VkFormat format, VkImageTiling tiling,
                         VkImageUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags);
        
        void change_image_layout(VkCommandPool commandPool, VkQueue queue, VkImage image,
                               VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
        
        bool is_stencil_format(VkFormat format);
        
        virtual void destroy() override;
        
        virtual ~image(){}
        
        void set_device(device* device){ _device = device;}
        
        inline VkImage get_image()
        {
            return _image;
        }
        
        inline VkImageView get_image_view()
        {
            return _image_view;
        }
        
        inline VkSampler get_sampler()
        {
            return _sampler;
        }
        
        inline int get_channels()
        {
            assert(_channels == 4);
            return _channels;
        }
        
        inline int get_width()
        {
            return _width;
        }
        
        inline int get_height()
        {
            return _height;
        }
        

        
        device*         _device = nullptr;
        VkImage         _image =        VK_NULL_HANDLE;
        VkDeviceMemory  _image_memory =  VK_NULL_HANDLE;
        VkImageView     _image_view =   VK_NULL_HANDLE;
        
        enum class formats
        {
            R8G8B8A8 = VK_FORMAT_R8G8B8A8_UNORM,
            DEPTH_32_FLOAT = VK_FORMAT_D32_SFLOAT,
            DEPTH_32_STENCIL_8 = VK_FORMAT_D32_SFLOAT_S8_UINT,
            DEPTH_24_STENCIL_8 = VK_FORMAT_D24_UNORM_S8_UINT
        };
        
        enum class filter
        {
            LINEAR = VkFilter::VK_FILTER_LINEAR,
            NEAREST = VkFilter::VK_FILTER_NEAREST
        };

        
        inline formats get_format()
        {
            return _format;
        }
        
        void set_filter( image::filter filter){ _filter = filter; }
        
        virtual const void* const get_instance_type() = 0;
        virtual void init() = 0;
    protected:
        
        formats _format = formats::R8G8B8A8;
        filter  _filter = filter::LINEAR;
        
        virtual void create_sampler() = 0;
        virtual void create_image_view( VkImage image, VkFormat format,
                                       VkImageAspectFlags aspectFlags, VkImageView& image_view) = 0;
        void write_buffer_to_image(VkCommandPool commandPool, VkQueue queue, VkBuffer buffer);
    
    protected:
        VkSampler _sampler = VK_NULL_HANDLE;
        //TODO: we only support 4 channels at the moment
        uint32_t _channels = 4;
        
        uint32_t _width = 0;
        uint32_t _height = 0;
        uint32_t _depth = 1;
    };
}


