//
//  texture_3d.hpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 7/18/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#pragma once


#include "image.h"


namespace vk {
    
    
    class texture_3d : public image
    {
    public:
        texture_3d(device* device, uint32_t width, uint32_t height, uint32_t depth):
        image(device)
        {
            _width = width;
            _height = height;
            _depth = depth;
        }

        texture_3d():image(){};
        
        virtual void * const * const get_instance_type() override { return (& _image_type); }
        static void * const * const  get_class_type(){ return (& _image_type); }
        
        virtual void init() override;
    private:
        
        static constexpr void* _image_type = nullptr;
        
        virtual void create_sampler()  override;
        virtual void create_image_view( VkImage image, VkFormat format, VkImageView& image_view) override;

    };
}
