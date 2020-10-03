//
//  texture.hpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 2/10/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#pragma once

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "resource.h"
#include "image.h"

#include "stb_image.h"
namespace vk
{
    class texture_2d : public image
    {
    public:
        
        texture_2d(){};
        texture_2d(device* device);
        texture_2d(device* device, uint32_t width, uint32_t height, uint32_t depth = 1);
        texture_2d(device* device,const char* path);
        virtual void destroy() override;
        
        stbi_uc * get_raw()
        {
            return _ppixels;
        }
        
        inline void set_path(const char* path)
        {
            _path = path;
        }
        
        int get_bytes_per_channel()
        {
            switch(_format)
            {
                case formats::R8G8B8A8_UNSIGNED_NORMALIZED:
                case formats::R8G8B8_UNSIGNED_NORMALIZED:
                case formats::R8G8B8A8_SIGNED_NORMALIZED:
                case formats::R8G8_SIGNED_NORMALIZED:
                case formats::R8_UNSIGNED_NORMALIZED:
                {
                    return 1;
                }
                case formats::R32G32_SIGNED_FLOAT:
                case formats::R32G32B32_SIGNED_FLOAT:
                case formats::R32G32B32A32_SIGNED_FLOAT:
                {
                    return 4;
                }
                case formats::R16G16B16A16_UNSIGNED_NORMALIZED:
                {
                    return 2;
                }
                default:
                    EA_FAIL_MSG("unrecognized texture format");
            }
            return 0;
        }
        
        int get_size_in_bytes()
        {
            return get_width() * get_height() * get_channels() * get_bytes_per_channel();
        }
        
        void load(stbi_uc ** pixels, const char* path);
        
        virtual void create_image_view(VkImage image, VkFormat format, VkImageView& image_view) override;
        virtual void init() override;
        
        virtual  char const * const * get_instance_type() override { return ( &_image_type); };
        static char const * const *  get_class_type(){ return (&_image_type); }
        
        void generate_mipmaps(VkImage image, VkCommandPool command_pool, VkQueue queue,
                              int32_t width, int32_t height, int32_t depth = 1);
        
        
        virtual void generate_mipmaps( VkImage image, VkCommandBuffer& command_buffer,
                                    uint32_t width,  uint32_t height, uint32_t depth);
        
        inline void refresh_mipmaps(VkCommandBuffer& command_buffer)
        {
            generate_mipmaps(_image, command_buffer, _width, _height, _depth);
        }
        
        inline void refresh_mimaps()
        {
            generate_mipmaps(_image, _device->_graphics_command_pool, _device->_graphics_queue, _width, _height, _depth);
        }
        
        inline void set_enable_mipmapping(bool b)
        {
            _enable_mipmapping = b;
        }
        
        static const eastl::fixed_string<char, 250> texture_resource_path;
        
    protected:
        
        virtual void create( uint32_t width, uint32_t height);
        virtual void create_sampler() override;
        bool _enable_mipmapping = false;
        eastl::fixed_string<char, 250> _path;
        bool _loaded = false;
    private:
        
        static constexpr char const * const _image_type = nullptr;
        stbi_uc *_ppixels = nullptr;
    };
}

