//
//  texture_cube.h
//  vulkan-demos
//
//  Created by Rafael Sabino on 8/4/20.
//  Copyright © 2020 Rafael Sabino. All rights reserved.
//

#pragma once

#include "texture_2d.h"
#include "EASTL/array.h"
namespace vk {

    class texture_cube : public texture_2d
    {
    public:
        
        texture_cube()
        {
        };
        
        //TODO: we need a constructor that can build a cube map from 6 images
        //TODO: start here: https://satellitnorden.wordpress.com/2018/01/23/vulkan-adventures-cube-map-tutorial/
        
        texture_cube(device* device): texture_2d(device)
        {
        };
        
        texture_cube(device* device, uint32_t width, uint32_t height):
        texture_2d(device, width, height, 6)
        {
        }
        
        virtual void set_dimensions(uint32_t width, uint32_t height, uint32_t ) override
        {
            EA_ASSERT_MSG(width == height, "width and height must be equal for cubemaps");
            _width = width;
            _height = height;
            //note: in this code, for cube maps, depth must be 6.  Always.
            _depth = 6;
        }

        virtual void generate_mipmaps( VkImage image, VkCommandBuffer& command_buffer,
                                    uint32_t width,  uint32_t height, uint32_t) override
        {
#if _APPLE_ && DEBUG
            EA_ASSERT_MSG( false, "moltenvk doesn't blit images very well, proceed with caution...");
#endif
            eastl::array<VkImageMemoryBarrier, 100> mip_barriers = {};
            EA_ASSERT(_mip_levels < 100);
            
            //note:: technically this could be done without a loop, but validation layers throw error when I do this, don't understand why...
            for(uint32_t i = 0; i < _mip_levels; ++i)
            {
                mip_barriers[i].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                mip_barriers[i].image = image;
                mip_barriers[i].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                mip_barriers[i].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                mip_barriers[i].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                mip_barriers[i].subresourceRange.baseMipLevel = i;
                mip_barriers[i].oldLayout = static_cast<VkImageLayout>(_image_layout);
                mip_barriers[i].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                mip_barriers[i].srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
                mip_barriers[i].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
                mip_barriers[i].subresourceRange.layerCount = _depth;
                mip_barriers[i].subresourceRange.levelCount = 1;
                mip_barriers[i].subresourceRange.baseArrayLayer = i * _depth;
            }

            vkCmdPipelineBarrier(command_buffer,
                                  VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0,
                                  0, nullptr,
                                  0, nullptr,
                                 _mip_levels, mip_barriers.data());
            
            VkImageMemoryBarrier barrier = {};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.image = image;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            
            int32_t mip_width = width;
            int32_t mip_height = height;
            EA_ASSERT_MSG(_depth == 6, "cubemaps must have depth of 6 for the the 6 layers");
            uint32_t i = 1;
            for (; i < _mip_levels; i++)
            {
                uint32_t prev_layer = (i - 1);
                barrier.subresourceRange.baseMipLevel = prev_layer;
                barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                barrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
                barrier.subresourceRange.layerCount = _depth;
                barrier.subresourceRange.levelCount = 1;
                barrier.subresourceRange.baseArrayLayer = (prev_layer * _depth);

                vkCmdPipelineBarrier(command_buffer,
                                  VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0,
                                  0, nullptr,
                                  0, nullptr,
                                  1, &barrier);

                //here we create the current mip level
                VkImageBlit blit = {};
                blit.srcOffsets[0] = {0, 0, 0};
                blit.srcOffsets[1] = {mip_width, mip_height, 1};
                blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                blit.srcSubresource.mipLevel =  prev_layer;
                //starting layer
                blit.srcSubresource.baseArrayLayer = prev_layer * _depth;
                //how many layers to copy
                blit.srcSubresource.layerCount = _depth;
                blit.dstOffsets[0] = {0, 0, 0};
                blit.dstOffsets[1] = { mip_width > 1 ? mip_width / 2 : 1, mip_height > 1 ? mip_height / 2 : 1, 1};

                blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                blit.dstSubresource.mipLevel = i;
                blit.dstSubresource.baseArrayLayer = i * _depth;
                blit.dstSubresource.layerCount = _depth;

                vkCmdBlitImage(command_buffer,
                            image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                            image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                            1, &blit,
                            VK_FILTER_LINEAR);

                //transfer the base miplevel to shader optimal
                barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                barrier.newLayout = static_cast<VkImageLayout>(_image_layout);;
                barrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

                vkCmdPipelineBarrier(command_buffer,
                                  VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0,
                                  0, nullptr,
                                  0, nullptr,
                                  1, &barrier);

                if (mip_width > 1) mip_width /= 2;
                if (mip_height > 1) mip_height /= 2;
            }
            
            barrier.subresourceRange.baseMipLevel = (i-1);
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = static_cast<VkImageLayout>(_image_layout);
            barrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            barrier.subresourceRange.layerCount = _depth;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.baseArrayLayer = (i-1) * _depth;
            
             
            vkCmdPipelineBarrier(command_buffer,
                                  VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0,
                                  0, nullptr,
                                  0, nullptr,
                                  1, &barrier);
            
            _image_layout = _original_layout;
        }
        
        virtual image_layouts get_usage_layout( vk::usage_type usage) override
        {
            image::image_layouts layout = get_original_layout();
            if(vk::usage_type::INPUT_ATTACHMENT == usage)
            {
                layout = image::image_layouts::COLOR_ATTACHMENT_OPTIMAL;
            }
            if(vk::usage_type::COMBINED_IMAGE_SAMPLER == usage)
            {
                layout = image::image_layouts::SHADER_READ_ONLY_OPTIMAL;
            }
            if(vk::usage_type::STORAGE_IMAGE == usage)
            {
                layout = image::image_layouts::GENERAL;
            }
            
            return layout;
        }
        
        virtual void create( uint32_t width, uint32_t height) override
        {
            EA_ASSERT(width != 0 && height != 0 );
            _width = width;
            _height = height;
            //note: cubemaps must have a depth of 6;
            _depth = 6;
            
            _image_layout = image_layouts::UNDEFINED;
            create_image(
                         static_cast<VkFormat>(_format),
                         VK_IMAGE_TILING_OPTIMAL,
                         VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT  | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                         VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, false);

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
            image_create_info.arrayLayers = _depth * _mip_levels;
            image_create_info.samples = _multisampling ? _device->get_max_usable_sample_count() : VK_SAMPLE_COUNT_1_BIT ;
            image_create_info.tiling = tiling;
            image_create_info.usage = usage_flags;

            return image_create_info;
        }
        
        VkImageView get_face_image_view(uint32_t face_id)
        {
            EA_ASSERT(_depth == 6);
            EA_ASSERT(face_id < _depth);
            
            if(_face_views[face_id] == 0)
            {
                VkImageViewCreateInfo image_view_create_info {};
                
                image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                image_view_create_info.pNext = nullptr;
                image_view_create_info.flags = 0;
                image_view_create_info.image = _image;
                image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
                image_view_create_info.format = static_cast<VkFormat>(_format);
                image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
                image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
                image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
                image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
                image_view_create_info.subresourceRange.aspectMask = _aspect_flag;
                image_view_create_info.subresourceRange.baseMipLevel = 0;
                image_view_create_info.subresourceRange.levelCount = _mip_levels;
                image_view_create_info.subresourceRange.baseArrayLayer = face_id;
                image_view_create_info.subresourceRange.layerCount = 1;
                
                VkResult result = vkCreateImageView(_device->_logical_device, &image_view_create_info, nullptr, &_face_views[face_id]);
                ASSERT_VULKAN(result);
            }
            return _face_views[face_id];
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
            image_view_create_info.subresourceRange.layerCount = _mip_levels * _depth;
            
            VkResult result = vkCreateImageView(_device->_logical_device, &image_view_create_info, nullptr, &image_view);
            ASSERT_VULKAN(result);
        }
        
        virtual void destroy() override
        {
            for( int i = 0; i < _depth; ++i)
            {
                if(_face_views[i] != VK_NULL_HANDLE)
                    vkDestroyImageView(_device->_logical_device, _face_views[i], nullptr);
            }
            texture_2d::destroy();
        }
        
        virtual char const * const * get_instance_type() override { return (& _image_type); }
        static char  const * const * get_class_type(){ return (& _image_type); }

    private:
        eastl::array<VkImageView, 6> _face_views = {};
        static constexpr const char * _image_type = nullptr;
    
    };
}
