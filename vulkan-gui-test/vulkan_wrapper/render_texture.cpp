//
//  render_texture.cpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 4/17/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#include "render_texture.h"
#include "device.h"
#include "texture_2d.h"

using namespace vk;


render_texture::render_texture(device* device, uint32_t width, uint32_t height, render_texture::usage intended_use):
texture_2d(device, width, height)
{
    render_texture::_usage = intended_use;
    
    _format = formats::R8G8B8A8;
    if( render_texture::_usage != render_texture::usage::COLOR_TARGET)
    {
        _format = static_cast<vk::image::formats>(device->find_depth_format());
    }
}


void render_texture::create(uint32_t width, uint32_t height)
{
    create_image(_width,
                 _height,
                 static_cast<VkFormat>(_format),
                 VK_IMAGE_TILING_OPTIMAL,
                 static_cast<VkImageUsageFlagBits>(_usage),
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    

    VkImageAspectFlags aspect_flag = (_format == formats::R8G8B8A8 ) ?
                    VK_IMAGE_ASPECT_COLOR_BIT : VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    
    
    create_image_view(_image, static_cast<VkFormat>(_format), aspect_flag, _image_view);
}

