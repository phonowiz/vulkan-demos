//
//  texture_cube.h
//  vulkan-demos
//
//  Created by Rafael Sabino on 8/4/20.
//  Copyright © 2020 Rafael Sabino. All rights reserved.
//

#pragma once

#include "texture_2d.h"
namespace vk {

    class texture_cube : texture_2d
    {
        
    public:
        
        texture_cube(){};
        
        texture_cube(device* device): texture_2d(device) {};
        
        texture_cube(device* device, uint32_t width, uint32_t height, uint32_t depth):
        texture_2d(device, width, height, depth)
        {}
        
        //TODO: we need a constructor that can build a cube map from 6 images
        //TODO: start here: https://satellitnorden.wordpress.com/2018/01/23/vulkan-adventures-cube-map-tutorial/
        
        virtual VkImageCreateInfo get_image_create_info( VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage_flags) override
        {
            VkImageCreateInfo image_create_info = {};
            
            image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            image_create_info.pNext  = nullptr;
            image_create_info.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
            image_create_info.imageType = VK_IMAGE_TYPE_2D;
            image_create_info.format = format;
            image_create_info.extent.width = _width;
            image_create_info.extent.height = _height;
            image_create_info.extent.depth = _depth;
            image_create_info.mipLevels = _mip_levels;
            image_create_info.arrayLayers = 6;
            image_create_info.samples = _multisampling ? _device->get_max_usable_sample_count() : VK_SAMPLE_COUNT_1_BIT ;
            image_create_info.tiling = tiling;
            image_create_info.usage = usage_flags;
            
            return image_create_info;
        }
        virtual void create_image_view(VkImage image, VkFormat format, VkImageView& image_view) override
        {
            VkImageViewCreateInfo image_view_create_info {};
            
            image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            image_view_create_info.pNext = nullptr;
            image_view_create_info.flags = 0;
            image_view_create_info.image = image;
            image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
            image_view_create_info.format = format;
            image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            image_view_create_info.subresourceRange.aspectMask = _aspect_flag;
            image_view_create_info.subresourceRange.baseMipLevel = 0;
            image_view_create_info.subresourceRange.levelCount = _mip_levels;
            image_view_create_info.subresourceRange.baseArrayLayer = 0;
            image_view_create_info.subresourceRange.layerCount = 6;
            assert(_depth == 1);
            VkResult result = vkCreateImageView(_device->_logical_device, &image_view_create_info, nullptr, &image_view);
            ASSERT_VULKAN(result);
        }
        
        
        virtual char const * const * get_instance_type() override { return (& _image_type); }
        static char  const * const * get_class_type(){ return (& _image_type); }

    private:
        
        static constexpr const char * _image_type = nullptr;
    
    };
}
