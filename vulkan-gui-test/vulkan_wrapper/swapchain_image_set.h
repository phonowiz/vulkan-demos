//
//  color_image.hpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 2/28/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#pragma once

#include "texture_2d.h"
#include <array>

namespace vk
{
    
    class swapchain_image_set : private texture_2d
    {
    public:
        
        swapchain_image_set(){};
        
        void init(device* device, VkSwapchainKHR swapChain) { _device = device; _swapchain = swapChain; }
        void create_image_set();
        void create_image_views(VkFormat format);
        
        using texture_2d::set_device;
        using texture_2d::destroy;

        inline uint32_t get_image_count(){ return _image_count; }
        inline VkImageView* get_image_views(){ return _imageViews.data() ;}
        inline VkImage*     get_images(){ return _images.data(); }
        
        //todo: the following two should be private
        //virtual void create(uint32_t width, uint32_t height) override {};

        virtual void destroy() override;
        
        static const int                                MAX_SWAP_CHAIN_IMAGES =     4;
        std::array<VkImage, MAX_SWAP_CHAIN_IMAGES>      _images;
        std::array<VkImageView, MAX_SWAP_CHAIN_IMAGES>  _imageViews;
        VkSwapchainKHR                                  _swapchain = VK_NULL_HANDLE;
        uint32_t                                        _image_count = 0;
        
    private:
        //inline void create_sampler()override{}
    };
}
