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
        
        enum class image_layouts;
        
        image(){}
        image( device* device){ _device = device; }
        void create_image( VkFormat format, VkImageTiling tiling,
                          VkImageUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags);
        
        void change_image_layout(VkCommandPool commandPool, VkQueue queue, VkImage image,
                                 VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
        
        bool is_stencil_format(VkFormat format);
        
        virtual void destroy() override;
        virtual VkImageCreateInfo get_image_create_info( VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage_flags);
        
        virtual ~image(){}
        
        void set_device(device* device){ _device = device;}
        
        inline VkImage get_image()
        {
            return _image;
        }
        
        inline uint32_t get_layer_count()
        {
            return _depth;
        }
        
        inline uint32_t get_mip_map_levels()
        {
            return _mip_levels;
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
            return _channels;
        }
        
        void set_dimensions(uint32_t width, uint32_t height, uint32_t depth = 1)
        {
            _width = width;
            _height = height;
            _depth = depth;
        }
        
        inline void change_layout( image::image_layouts new_layout)
        {
            change_image_layout(_device->_graphics_command_pool, _device->_graphics_queue, _image, static_cast<VkFormat>(_format),
                                      static_cast<VkImageLayout>(_image_layout),
                                      static_cast<VkImageLayout>(new_layout));
        }
        
        inline uint32_t get_width()
        {
            return _width;
        }
        
        inline uint32_t get_height()
        {
            return _height;
        }
        
        inline uint32_t get_depth()
        {
            return _depth;
        }
        
        inline bool is_initialized(){ return _image != VK_NULL_HANDLE; }
    
        
        device*         _device = nullptr;
        VkImage         _image =        VK_NULL_HANDLE;
        VkDeviceMemory  _image_memory =  VK_NULL_HANDLE;
        VkImageView     _image_view =   VK_NULL_HANDLE;
        
        //note: if adding new formats, please make sure to adjust the set_format function
        //to automatically set the right number of necessary channels
        enum class formats
        {
            R8G8B8A8_UNSIGNED_NORMALIZED = VK_FORMAT_R8G8B8A8_UNORM,
            R32G32_SIGNED_FLOAT = VK_FORMAT_R32G32_SFLOAT,
            R32G32B32A32_SIGNED_FLOAT = VK_FORMAT_R32G32B32A32_SFLOAT,
            R16G16B16A16_UNSIGNED_NORMALIZED = VK_FORMAT_R16G16B16A16_UNORM,
            R8G8B8A8_SIGNED_NORMALIZED = VK_FORMAT_R8G8B8A8_SNORM,
            DEPTH_32_FLOAT = VK_FORMAT_D32_SFLOAT,
            DEPTH_32_STENCIL_8 = VK_FORMAT_D32_SFLOAT_S8_UINT,
            DEPTH_24_STENCIL_8 = VK_FORMAT_D24_UNORM_S8_UINT,
            R8G8_SIGNED_NORMALIZED =  VK_FORMAT_R8G8_SNORM 
        };
        
        enum class image_layouts
        {
            PREINITIALIZED = VK_IMAGE_LAYOUT_PREINITIALIZED,
            UNDEFINED = VK_IMAGE_LAYOUT_UNDEFINED,
            GENERAL = VK_IMAGE_LAYOUT_GENERAL,
            DEPTH_STENCIL_ATTACHMENT_OPTIMAL = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            DEPTH_STENCIL_READ_ONLY_OPTIMAL = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
            SHADER_READ_ONLY_OPTIMAL = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            TRANSFER_SOURCE_OPTIMAL = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            TRANSFER_DESTINATION_OPTIMAL = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            COLOR_ATTACHMENT_OPTIMAL = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            PRESENT_KHR = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
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
        
        inline VkImageAspectFlags get_aspect_flag()
        {
            return _aspect_flag;
        }
        
        virtual void set_format(formats f)
        {
            set_channels(4);
            _aspect_flag = VK_IMAGE_ASPECT_COLOR_BIT;
            
            if(formats::R8G8_SIGNED_NORMALIZED == f)
            {
                set_channels(2);
            }
            _format = f;
        }
        
        inline void set_native_layout( image_layouts l)
        {
            _image_layout = l;
        }
        
        inline void set_image_layout( image_layouts layout)
        {
            _image_layout = layout;
        }
        
        virtual void reset_image_layout()
        {
            if(_original_layout != _image_layout)
                change_layout( _original_layout);
            
        }
        
        inline image_layouts get_original_layout()
        {
            return _original_layout;
        }
        
        image_layouts get_native_layout()
        {
            return _image_layout;
        }
        
        virtual image_layouts get_usage_layout( vk::usage_type usage)
        {
            image::image_layouts layout = get_native_layout();
            if(vk::usage_type::INPUT_ATTACHMENT == usage)
            {
                layout = image::image_layouts::SHADER_READ_ONLY_OPTIMAL;
            }
            
            return layout;
        }
        void set_filter( image::filter filter){ _filter = filter; }
        
        //inline bool is_initialized(){ return _initialized; }
        virtual void init() = 0;
    
    protected:
        
        inline void set_channels(uint32_t channels)
        {
            _channels = channels;
        }
        
        virtual void create_sampler() = 0;
        virtual void create_image_view( VkImage image, VkFormat format, VkImageView& image_view) = 0;
        void write_buffer_to_image(VkCommandPool commandPool, VkQueue queue, VkBuffer buffer);
        
    protected:
        VkSampler _sampler = VK_NULL_HANDLE;
        VkImageAspectFlags _aspect_flag = VK_IMAGE_ASPECT_COLOR_BIT;
        uint32_t _channels = 4;
        
        uint32_t _width = 0;
        uint32_t _height = 0;
        uint32_t _depth = 1;
        uint32_t _mip_levels = 1;
        bool _initialized = false;
        formats _format = formats::R8G8B8A8_SIGNED_NORMALIZED;
        filter  _filter = filter::LINEAR;
        image_layouts _image_layout = image_layouts::UNDEFINED;
        image_layouts _original_layout = image_layouts::UNDEFINED;
        
    public:
        inline image::filter get_filter()
        {
            return _filter;
        }
    };
}


