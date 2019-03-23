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
    
    class device;
    
    class depth_image : public image
    {
    public:
        
        depth_image(){};
        depth_image(device* physicalDevice):image(physicalDevice){}
        
        virtual void create(uint32_t width, uint32_t height) override;
        virtual void destroy() override;
        VkAttachmentDescription getDepthAttachment();
        
        bool _created = false;
        
        
        depth_image(const depth_image&) = delete;
        depth_image(depth_image&&) = delete;
        depth_image& operator=(const depth_image &) = delete;
        depth_image& operator=(depth_image&&) = delete;
    };
}



