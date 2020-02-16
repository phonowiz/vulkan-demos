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


render_texture::render_texture(device* device, uint32_t width, uint32_t height):
texture_2d(device, width, height)
{
}


void render_texture::create(uint32_t width, uint32_t height)
{

    create_image(
                 static_cast<VkFormat>(_format),
                 VK_IMAGE_TILING_OPTIMAL,
                 static_cast<VkImageUsageFlagBits>(_usage),
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    

    create_image_view(_image, static_cast<VkFormat>(_format), _image_view);
    
    change_image_layout(_device->_graphics_command_pool, _device->_graphics_queue, _image, static_cast<VkFormat>(_format),
                              static_cast<VkImageLayout>(_image_layout),
                              static_cast<VkImageLayout>(image_layouts::COLOR_ATTACHMENT_OPTIMAL));
}

