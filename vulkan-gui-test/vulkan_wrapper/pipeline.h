//
//  pipeline.hpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 6/30/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#pragma once

#include "device.h"

namespace vk
{
    class pipeline
    {
    public:
        
        pipeline( device* device)
        {
            _device = device;
        }
        
        virtual ~pipeline(){};
        
        
        VkPipeline _pipeline = VK_NULL_HANDLE;
        VkPipelineLayout _pipeline_layout = VK_NULL_HANDLE;
        device* _device = nullptr;
        
    };
}
