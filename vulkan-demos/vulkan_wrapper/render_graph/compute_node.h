//
//  compute_node.hpp
//  vulkan-demos
//
//  Created by Rafael Sabino on 4/7/20.
//  Copyright © 2020 Rafael Sabino. All rights reserved.
//

#pragma once

#include "glfw_swapchain.h"
#include "EASTL/array.h"
#include "compute_pipeline.h"
#include "command_recorder.h"

namespace vk
{
    template< uint32_t NUM_CHILDREN >
    class compute_node : public node<NUM_CHILDREN>
    {
    public:
        
        using node_type = node<NUM_CHILDREN>;
        using compute_pipeline_type = compute_pipeline<vk::NUM_SWAPCHAIN_IMAGES>;
        compute_node(){};
        
        compute_node(device* dev, uint32_t local_group_x, uint32_t local_group_y, uint32_t local_group_z = 1u):
        node_type::node_type(dev),
        _group_x(local_group_x),
        _group_y(local_group_y),
        _group_z(local_group_z)
        {
            for( int i = 0; i < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++i)
            {
                _compute_pipelines.set_device(dev);
            }
        }
        
        
        virtual void init() override
        {
            _compute_pipelines.set_device(node_type::_device);
            node_type::init();

        }
        
        inline void set_group_size(uint32_t group_x, uint32_t group_y, uint32_t group_z)
        {
            _group_x = group_x;
            _group_y = group_y;
            _group_z = group_z;
        }
        virtual bool record_node_commands(command_recorder& buffer, uint32_t image_id) override
        {
            assert(node_type::_device != nullptr && "no device set for this node");
            _compute_pipelines.set_device(node_type::_device);
            _compute_pipelines.record_dispatch_commands(buffer.get_raw_compute_command(image_id), image_id,
                                                              _group_x, _group_y, _group_z);
            return true;
        }
        
        virtual void destroy() override
        {
            _compute_pipelines.destroy();
        }
        
        VkPipelineStageFlagBits get_producer_stage() override {  return VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT; };
        VkPipelineStageFlagBits get_consumer_stage() override {  return VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT; };
        
    protected:
        virtual void create_gpu_resources() override
        {
            for(int i = 0; i < vk::NUM_SWAPCHAIN_IMAGES; ++i)
            {
                //committing for the first time will create gpu resources
                _compute_pipelines.commit_parameter_to_gpu(i);
            }
        }
        
        
        compute_pipeline<vk::NUM_SWAPCHAIN_IMAGES> _compute_pipelines;
        
        //note:: 8 is chosen here because that's the max number allowed on my macbook pro mid 2014
        //TODO: find out max group size using api
        uint32_t _group_x = 8;
        uint32_t _group_y = 8;
        uint32_t _group_z = 8;
    };
}
