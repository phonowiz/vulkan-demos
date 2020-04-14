//
//  render_texture.hpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 4/17/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#pragma once

#include "texture_2d.h"


namespace vk
{
    class render_texture : public texture_2d
    {
    public:
        
        enum class usage
        {
            COLOR_TARGET = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
            COLOR_TARGET_AND_SHADER_INPUT = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
        };
        
        render_texture(){ _usage = render_texture::usage::COLOR_TARGET_AND_SHADER_INPUT;};
        render_texture(device* device, uint32_t width, uint32_t height);
        
        virtual char const * const * get_instance_type() override { return (&_image_type); };
        static char const * const *  get_class_type(){ return (&_image_type); }

        virtual void create( uint32_t width, uint32_t height) override;
        
    private:
        
        static constexpr char const * _image_type = nullptr;
        render_texture::usage _usage = usage::COLOR_TARGET;
    };
}
