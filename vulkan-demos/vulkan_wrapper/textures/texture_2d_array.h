//
//  texture_2d_array.h
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 11/7/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#pragma once

#include "texture_2d.h"

namespace vk {
    class texture_2d_array: public texture_2d
    {
    public:
        texture_2d_array(device* device, uint32_t width, uint32_t height, uint32_t depth) :
            texture_2d(device, width, height)
        {
            _depth = depth;
        }
        
        virtual void init() override;
        
        virtual char const * const * get_instance_type() override { return ( &_image_type) ; }
        static char const * const  * get_class_type(){ return ( &_image_type) ; }
        virtual VkImageCreateInfo get_image_create_info( VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage_flags) override;
    private:
        
        static constexpr char const* const _image_type = nullptr;
        
        virtual void create_sampler()  override;
        virtual void create_image_view( VkImage image, VkFormat format, VkImageView& image_view) override;

    };
}
