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
        
        bool is_initialized()
        {
            bool result = true;
            for( int i =0; i < _pipeline.size(); ++i)
            {
                if(_pipeline[i] == VK_NULL_HANDLE || _pipeline_layout[i] == VK_NULL_HANDLE)
                {
                    result = false;
                    break;
                }
            }
            return result;
        }
        
        virtual void commit_parameters_to_gpu(uint32_t swapchain_id) = 0;
        
        virtual void destroy()
        {
            for(int i = 0; i < _pipeline.size(); ++i)
            {
                vkDestroyPipeline(_device->_logical_device, _pipeline[i], nullptr);
                vkDestroyPipelineLayout(_device->_logical_device, _pipeline_layout[i], nullptr);
                
                _pipeline[i] = VK_NULL_HANDLE;
                _pipeline_layout[i] = VK_NULL_HANDLE;
            }
        }
        
    public:
        std::array<VkPipeline, glfw_swapchain::NUM_SWAPCHAIN_IMAGES >       _pipeline {};
        std::array<VkPipelineLayout, glfw_swapchain::NUM_SWAPCHAIN_IMAGES>  _pipeline_layout {};
    protected:
        device* _device = nullptr;
        
    };
}
