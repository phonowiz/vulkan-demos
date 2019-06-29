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
    
    class depth_image : public texture_2d
    {
    public:
        
        depth_image(bool write_to_texture){ _write_to_texture = write_to_texture; };
        depth_image(vk::device* device, bool write_to_texture ):texture_2d(device){ _write_to_texture = write_to_texture; }
        
        virtual void create(uint32_t width, uint32_t height) override;
        virtual void destroy() override;
        virtual void create_sampler() override;
        VkAttachmentDescription get_depth_attachment();
        
    protected:
        bool _created = false;
        bool _write_to_texture = false;
        
        depth_image(const depth_image&) = delete;
        depth_image(depth_image&&) = delete;
        depth_image& operator=(const depth_image &) = delete;
        depth_image& operator=(depth_image&&) = delete;
    };
}



