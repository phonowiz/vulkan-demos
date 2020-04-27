//
//  depth_image.hpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 2/17/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#pragma once

#include "texture_2d.h"


namespace vk {
    
    class device;
    
    class depth_texture : public texture_2d
    {
    public:
        
        depth_texture(){};
        depth_texture(bool write_to_texture){ _write_to_texture = write_to_texture; };
        depth_texture(vk::device* device, uint32_t width, uint32_t height, bool write_to_texture )
            :texture_2d(device, width, height){ _write_to_texture = write_to_texture; }
        

        virtual void init() override;
        virtual void destroy() override;
        virtual void create_sampler() override;
        virtual void set_format( formats f) override;
        virtual void set_write_to_texture(bool write){ _write_to_texture = write; };
        
        
        static  char const * const *  get_class_type(){ return (&_image_type); }
        virtual char const * const *  get_instance_type() override { return (&_image_type); };
        
        virtual image_layouts get_usage_layout( resource::usage_type usage) override
        {
            image::image_layouts layout = texture_2d::get_usage_layout(usage);
            
            if(usage == resource::usage_type::INPUT_ATTACHMENT)
            {
                layout = image::image_layouts::DEPTH_STENCIL_READ_ONLY_OPTIMAL;
            }
            if(usage == resource::usage_type::STORAGE_IMAGE)
            {
                //TODO: IS THERE A BETTER ONE? 
                layout = image::image_layouts::DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            }
            
            return layout;
        }
        
        VkAttachmentDescription get_depth_attachment();
        
        
        
    private:
        static constexpr char const * _image_type = nullptr;
    protected:
        bool _created = false;
        bool _write_to_texture = false;
        virtual void create(uint32_t width, uint32_t height) override;
        
        depth_texture(const depth_texture&) = delete;
        depth_texture(depth_texture&&) = delete;
        depth_texture& operator=(const depth_texture &) = delete;
        depth_texture& operator=(depth_texture&&) = delete;
    };
}



