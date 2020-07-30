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
        
        render_texture()
        {
            _original_layout = image::image_layouts::SHADER_READ_ONLY_OPTIMAL;
        };
        render_texture(device* device, uint32_t width, uint32_t height);
        
        
        virtual char const * const * get_instance_type() override { return (&_image_type); };
        static char const * const *  get_class_type(){ return (&_image_type); }

        virtual image_layouts get_usage_layout( vk::usage_type usage) override
        {
            image::image_layouts layout = texture_2d::get_usage_layout(usage);
            
            if(usage == vk::usage_type::COMBINED_IMAGE_SAMPLER)
            {
                layout = image::image_layouts::SHADER_READ_ONLY_OPTIMAL;
            }
            if(usage == vk::usage_type::INPUT_ATTACHMENT)
            {
                layout = image::image_layouts::SHADER_READ_ONLY_OPTIMAL;
            }
            
            return layout;
        }
        
        virtual void reset_image_layout() override
        {
            //this texture relies on render passes to reset the layout...
            //unlike other textures that do it themselves
            _image_layout = _original_layout;
            
        }
        
        virtual void create( uint32_t width, uint32_t height) override;
        
    private:
        
        static constexpr char const * _image_type = nullptr;
        //render_texture::usage _usage = usage::COLOR_TARGET;
    };
}
