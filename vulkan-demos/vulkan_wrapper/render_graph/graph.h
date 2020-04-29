//
//  graph.hpp
//  vulkan-demos
//
//  Created by Rafael Sabino on 4/4/20.
//  Copyright © 2020 Rafael Sabino. All rights reserved.
//

#pragma once

#include "texture_registry.h"
#include "command_recorder.h"
#include "EASTL/stack.h"
#include "EASTL/fixed_vector.h"
#include "graphics_node.h"
#include "compute_node.h"
#include "command_recorder.h"

namespace vk
{
    //based off of frame graph implemented in the frostbite engine: https://www.bilibili.com/video/av10595011/
    template<size_t NUM_CHILDREN>
    class graph : protected node<NUM_CHILDREN>
    {
    public:
        
        using node_type = vk::node<NUM_CHILDREN>;
        
        graph(device* dev, material_store& mat_store, glfw_swapchain& swapchain):
        node_type::node_type(dev),
        _commands(dev, swapchain),
        _material_store(mat_store)
        {
            node_type::_device = dev;
            node<NUM_CHILDREN>::_name = "root";
            node_type::set_stores(_texture_store, _material_store);
        }
        
        
        
        bool validate();
        
        using node_type::add_child;
        
        virtual void init() override
        {
            node_type::pre_init(node_type::_level, node_type::_device);
            create_gpu_resources();

            for( eastl_size_t i = 0; i < node_type::_children.size(); ++i)
            {
                node_type::_children[i]->set_device(node_type::_device);
                node_type::_children[i]->set_stores(_texture_store, _material_store);
                node_type::_children[i]->init();
            }
            
            init_node();

        }
        
        inline void record(uint32_t image_id)
        {
            _commands.reset(image_id);
            _commands.begin_command_recording(image_id);
            record(_commands, image_id);
            _commands.end_command_recording(image_id);
        }
        
        //submits all commands
        virtual void execute(uint32_t image_id)
        {
            _commands.submit_graphics_commands(image_id);
        }
        
        virtual void update(vk::camera& camera, uint32_t image_id) override
        {
            for( eastl_size_t i = 0; i < node_type::_children.size(); ++i)
            {
                node_type::_children[i]->update(camera,  image_id);
            }
        }
        
        void destroy() override
        {
            _commands.destroy();
            _texture_store.destroy();
        }
        
    protected:
        
        virtual void create_gpu_resources() override
        {}
        
        virtual void record_node_commands(command_recorder& buffer, uint32_t image_id) override
        {}
        
        virtual void init_node() override
        {}
        
        virtual VkPipelineStageFlagBits get_producer_stage() override {return VK_PIPELINE_STAGE_FLAG_BITS_MAX_ENUM;};
        virtual VkPipelineStageFlagBits get_consumer_stage() override { return VK_PIPELINE_STAGE_FLAG_BITS_MAX_ENUM;};
        
        virtual void record(command_recorder& buffer, uint32_t image_id) override
        {
            node_type::record(buffer, image_id);
        }
    private:
        
        texture_registry<NUM_CHILDREN> _texture_store;
        command_recorder _commands;
        material_store& _material_store;
        
        vk::device device;
        
    };
}

