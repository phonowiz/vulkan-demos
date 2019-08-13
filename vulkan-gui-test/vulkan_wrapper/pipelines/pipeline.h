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

namespace vk
{
    class pipeline : public object
    {
    public:
        
        pipeline( device* device)
        {
            _device = device;
        }
        
        virtual ~pipeline(){};
        
        inline void destroy()
        {
            vkDestroyPipeline(_device->_logical_device, _pipeline, nullptr);
            vkDestroyPipelineLayout(_device->_logical_device, _pipeline_layout, nullptr);
            _pipeline = VK_NULL_HANDLE;
            _pipeline_layout = VK_NULL_HANDLE;
        }

        
        VkPipeline _pipeline = VK_NULL_HANDLE;
        VkPipelineLayout _pipeline_layout = VK_NULL_HANDLE;
        device* _device = nullptr;
        
    };
}
