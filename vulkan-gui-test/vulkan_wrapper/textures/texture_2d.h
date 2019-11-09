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
        texture_2d(device* device, uint32_t width, uint32_t height);
        texture_2d(device* device,const char* path);
        
        virtual void create( uint32_t width, uint32_t height);
        virtual void destroy() override;
        
        stbi_uc * get_raw()
        {
            return _ppixels;
        }
        
        int get_size_in_bytes()
        {
            return get_width() * get_height() * get_channels();
        }
        
        void load();
        virtual void create_image_view(VkImage image, VkFormat format, VkImageAspectFlags aspect_flags, VkImageView& image_view) override;
        virtual void init() override;
        
        virtual const void* const get_instance_type() override { return _image_type; };
        static const void* const  get_class_type(){ return _image_type; }
        
        void generate_mipmaps(VkImage image, VkCommandPool command_pool, VkQueue queue,
                              int32_t width, int32_t height, int32_t depth = 1);
        
        inline void refresh_mimaps()
        {
            generate_mipmaps(_image, _device->_graphics_command_pool, _device->_graphics_queue, _width, _height, _depth);
        }
        
        inline void set_enable_mipmapping(bool b)
        {
            _enable_mipmapping = b;
        }
        
        static const std::string texture_resource_path;
        
    protected:
        virtual void create_sampler() override;
        bool _enable_mipmapping = false;
    private:
        
        static constexpr void* _image_type = nullptr;
        
        bool _loaded = false;
        bool _initialized = false;
        
        stbi_uc *_ppixels = nullptr;
        std::string _path;
    };
}

