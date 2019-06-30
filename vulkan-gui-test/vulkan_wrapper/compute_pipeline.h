//
//  compute_pipeline.hpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 6/29/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#pragma once

#include "texture_2d.h"

namespace vk
{
    class texture_2d;
    
    class compute_pipeline
    {
    public:
//        compute_pipeline(device* device, material_shared_ptr material )
//        {
//            _device = device;
//            _material = material;
//        };
//        
//        compute_pipeline(){};
        
    public:
        VkPipeline _pipeline = VK_NULL_HANDLE;
        
        device* _device = nullptr;
        
    };
}

