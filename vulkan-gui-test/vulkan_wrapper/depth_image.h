//
//  depth_image.hpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 2/17/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#pragma once

#include "image.h"


namespace vk {
    
    class PhysicalDevice;
    
    class DepthImage : public Image
    {
    public:
        
        DepthImage(){};
        DepthImage(PhysicalDevice* physicalDevice):Image(physicalDevice){}
        
        virtual void create(uint32_t width, uint32_t height);
        virtual void destroy() override;
        VkAttachmentDescription getDepthAttachment();
        
        bool _created = false;
        
        
        DepthImage(const DepthImage&) = delete;
        DepthImage(DepthImage&&) = delete;
        DepthImage& operator=(const DepthImage &) = delete;
        DepthImage& operator=(DepthImage&&) = delete;
    };
}



