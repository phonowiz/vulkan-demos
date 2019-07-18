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
        
        virtual void create( uint32_t width, uint32_t height) override;
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
        
        virtual void create_sampler() override;
        virtual void create_image_view(VkImage image, VkFormat format, VkImageAspectFlags aspect_flags, VkImageView &image_view) override;
        
        void set_sampler(device* device){ }
        
        static const std::string texture_resource_path;
    private:
        
        bool _loaded = false;
        bool _uploaded = false;
        
        stbi_uc *_ppixels = nullptr;
        std::string _path;
    };
}

