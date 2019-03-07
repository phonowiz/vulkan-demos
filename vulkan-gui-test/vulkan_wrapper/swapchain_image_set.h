//
//  color_image.hpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 2/28/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#pragma once

#include "image.h"
#include <array>

namespace vk
{
    
    class SwapchainImageSet : private Image
    {
    public:
        
        SwapchainImageSet(){};
        
        void init(PhysicalDevice* device, VkSwapchainKHR swapChain) { _device = device; _swapChain = swapChain; }
        void createImageSet();
        void createImageViews(VkFormat format);
        
        using Image::setDevice;
        using object::destroy;
        
        inline uint32_t getImageCount(){ return _imageCount; }
        inline VkImageView* getImageViews(){ return _imageViews.data() ;}
        inline VkImage*     getImages(){ return _images.data(); }
        
        //todo: the following two should be private
        virtual void create(uint32_t width, uint32_t height) override {};
        virtual void destroy() override;
        
        static const int                                MAX_SWAP_CHAIN_IMAGES =     4;
        std::array<VkImage, MAX_SWAP_CHAIN_IMAGES>      _images;
        std::array<VkImageView, MAX_SWAP_CHAIN_IMAGES>  _imageViews;
        VkSwapchainKHR                                  _swapChain = VK_NULL_HANDLE;
        uint32_t                                        _imageCount = 0;
        
    };
}
