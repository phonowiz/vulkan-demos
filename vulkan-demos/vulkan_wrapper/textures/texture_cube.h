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

    class texture_cube : public texture_2d
    {
    public:
        
        texture_cube()
        {
            _original_layout = image_layouts::GENERAL;
        };
        
        //TODO: we need a constructor that can build a cube map from 6 images
        //TODO: start here: https://satellitnorden.wordpress.com/2018/01/23/vulkan-adventures-cube-map-tutorial/
        
        texture_cube(device* device): texture_2d(device)
        {
            _original_layout = image_layouts::GENERAL;
        };
        
        texture_cube(device* device, uint32_t width, uint32_t height):
        texture_2d(device, width, height, 6)
        {
            _original_layout = image_layouts::GENERAL;
        }
        
        virtual void set_dimensions(uint32_t width, uint32_t height, uint32_t ) override
        {
            _width = width;
            _height = height;
            //note: for cube maps, depth must be 6.  Always.
            _depth = 6;
        }

        
        virtual void create( uint32_t width, uint32_t height) override
        {
            EA_ASSERT(width != 0 && height != 0 );
            _width = width;
            _height = height;
            //note: cubemaps must have a depth of 6;
            _depth = 6;
            
//            VkDeviceSize image_size = get_size_in_bytes();
//            
//            
//            VkBuffer staging_buffer {};
//            VkDeviceMemory staging_buffer_memory {};
            
            
//            VkMemoryPropertyFlagBits flags =  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
//
//            create_buffer(_device->_logical_device, _device->_physical_device, image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
//                          staging_buffer, flags, staging_buffer_memory);
//
//
            
            create_image(
                         static_cast<VkFormat>(_format),
                         VK_IMAGE_TILING_OPTIMAL,
                         VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        
            VkCommandBuffer command_buffer =
                _device->start_single_time_command_buffer(_device->_graphics_command_pool);

            VkImageMemoryBarrier barrier = {};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.image = _image;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.oldLayout = static_cast<VkImageLayout>(_image_layout);
            barrier.newLayout = static_cast<VkImageLayout>(image_layouts::GENERAL);
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            barrier.subresourceRange.layerCount = _depth;
            barrier.subresourceRange.levelCount = 1;

            vkCmdPipelineBarrier(command_buffer,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrier);

            _image_layout = image_layouts::GENERAL;
            
            _device->
                end_single_time_command_buffer(_device->_graphics_queue, _device->_graphics_command_pool, command_buffer);
            
            create_image_view(_image, static_cast<VkFormat>(_format), _image_view);
            _initialized = true;
        }
        
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
            image_create_info.extent.depth = 1.0f;
            image_create_info.mipLevels = _mip_levels;
            image_create_info.arrayLayers = _depth;
            image_create_info.samples = _multisampling ? _device->get_max_usable_sample_count() : VK_SAMPLE_COUNT_1_BIT ;
            image_create_info.tiling = tiling;
            image_create_info.usage = usage_flags;
            
            return image_create_info;
        }
        virtual void create_image_view(VkImage image, VkFormat format, VkImageView& image_view) override
        {
            EA_ASSERT(_depth == 6);
            
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
            
            VkResult result = vkCreateImageView(_device->_logical_device, &image_view_create_info, nullptr, &image_view);
            ASSERT_VULKAN(result);
        }
        
//        virtual void texture_2d::create_sampler() override
//        {
//            VkSamplerCreateInfo sampler_create_info {};
//
//            sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
//            sampler_create_info.pNext = nullptr;
//            sampler_create_info.flags = 0;
//            sampler_create_info.magFilter = static_cast<VkFilter>(_filter);
//            sampler_create_info.minFilter = static_cast<VkFilter>(_filter);
//            sampler_create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
//            sampler_create_info.addressModeU  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
//            sampler_create_info.addressModeV =  VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
//            sampler_create_info.addressModeW =  VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
//            sampler_create_info.mipLodBias = 0.0f;
//            sampler_create_info.anisotropyEnable = VK_TRUE;
//            sampler_create_info.maxAnisotropy = 16;
//            sampler_create_info.compareEnable = VK_FALSE;
//            sampler_create_info.compareOp = VK_COMPARE_OP_ALWAYS;
//            sampler_create_info.minLod = 0.0f;
//            sampler_create_info.maxLod = static_cast<float>(_mip_levels);
//            sampler_create_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
//            sampler_create_info.unnormalizedCoordinates = VK_FALSE;
//
//            VkResult result = vkCreateSampler(_device->_logical_device, &sampler_create_info, nullptr, &_sampler);
//            ASSERT_VULKAN(result);
//        }
        
        
        virtual char const * const * get_instance_type() override { return (& _image_type); }
        static char  const * const * get_class_type(){ return (& _image_type); }

    private:
        
        static constexpr const char * _image_type = nullptr;
    
    };
}
