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
            DEPTH_TARGET = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
            NONE         = VK_IMAGE_USAGE_FLAG_BITS_MAX_ENUM
        };
        
        render_texture(device* device, uint32_t width, uint32_t height, render_texture::usage usage);

        virtual void create( uint32_t width, uint32_t height) override;
        
    private:
        
        render_texture::usage _usage = usage::NONE;
    };
}
