//
//  pipeline.hpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 6/30/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#pragma once

#include "device.h"
#include "object.h"
#include "glfw_swapchain.h"
namespace vk
{
    class pipeline : public object
    {
    public:
        
        pipeline(){}
        pipeline( device* device)
        {
            _device = device;
        }
        
        void set_device(device* dev){ _device = dev; }
        virtual ~pipeline(){};
        
        //virtual void commit_parameters_to_gpu(uint32_t image_id) = 0;
        
    public:

    protected:
        device* _device = nullptr;
        
    };
}
