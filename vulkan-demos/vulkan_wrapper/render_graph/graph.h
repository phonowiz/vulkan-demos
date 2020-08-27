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
        
        using tex_registry_type = texture_registry<NUM_CHILDREN>;
        using node_type = vk::node<NUM_CHILDREN>;
        
        graph(device* dev, material_store& mat_store, glfw_swapchain& swapchain):
        node_type::node_type(dev),
        _commands(dev, swapchain),
        _material_store(mat_store),
        _texture_registry(dev)
        {
            node_type::_device = dev;
            node<NUM_CHILDREN>::_name = "root";
            node_type::set_stores(_texture_registry, _material_store);
        }
        
        
        
        bool validate();
        
        using node_type::add_child;
        using node_type::destroy_all;
        
        virtual void init() override
        {
            node_type::reset_node(node_type::_level, node_type::_device);
            create_gpu_resources();

            for( eastl_size_t i = 0; i < node_type::_children.size(); ++i)
            {
                node_type::_children[i]->set_device(node_type::_device);
                node_type::_children[i]->set_stores(_texture_registry, _material_store);
                node_type::_children[i]->init();
            }
            
            init_node();

        }
        
        
        inline void record(uint32_t image_id)
        {
            node_type::reset_node(node_type::_level, node_type::_device);
            
            _commands.reset(image_id);
            _commands.begin_command_recording(image_id);
            record(_commands, image_id);
            _texture_registry.reset_render_textures(image_id);
            //reset_textures(_commands, image_id);
            _commands.end_command_recording(image_id);
        }
        
        //submits all commands
        virtual void execute(uint32_t image_id)
        {
            _commands.submit_graphics_commands(image_id);
        }
        
        void update(vk::camera& camera, uint32_t image_id) override
        {
            node_type::reset_node(node_type::_level, node_type::_device);
            
            for( eastl_size_t i = 0; i < node_type::_children.size(); ++i)
            {
                node_type::_children[i]->update(camera,  image_id);
            }
        }
        
        void destroy() override
        {
            _commands.destroy();
            _texture_registry.destroy();
        }
        
    protected:
        
        void reset_textures(command_recorder& buffer,  uint32_t image_id)
        {
            //typename tex_registry_type::node_dependees& dependees = _texture_registry->get_dependees(this);
            typename tex_registry_type::dependee_data_map::iterator iter = _texture_registry.get_dependees();
            typename tex_registry_type::dependee_data_map::iterator end = _texture_registry.get_dependees_end();
            //typename tex_registry_type::node_dependees::iterator begin = dependees.begin();
            //typename tex_registry_type::node_dependees::iterator end = dependees.end();
          
            //TODO: In theory we could collect all the barriers and have one vkCmdPipelineBarrier
            while( iter != end)
            {
                typename tex_registry_type::dependee_data& d = iter->second;
                eastl::shared_ptr<vk::object> res = eastl::static_pointer_cast<vk::object>(d.resource);
                
                vk::usage_transition current_trans = {};
                vk::usage_transition last_trans {};
                if(res->get_instance_type()  == resource_set<vk::texture_2d>::get_class_type())
                {
                    eastl::shared_ptr< resource_set<vk::texture_2d> > set = eastl::static_pointer_cast< resource_set<vk::texture_2d>>(res);
                    last_trans = (*set).get_last_transition();
                    current_trans =  (*set).get_current_transition();
                }
                else if(res->get_instance_type()  == resource_set<vk::texture_3d>::get_class_type())
                {
                    eastl::shared_ptr< resource_set<vk::texture_2d> > set = eastl::static_pointer_cast< resource_set<vk::texture_2d>>(res);
                    last_trans = (*set).get_last_transition();
                    current_trans =  (*set).get_current_transition();
                }
                else if(res->get_instance_type()  == resource_set<vk::depth_texture>::get_class_type())
                {
                    eastl::shared_ptr< resource_set<vk::texture_2d> > set = eastl::static_pointer_cast< resource_set<vk::texture_2d>>(res);
                    last_trans = (*set).get_last_transition();
                    current_trans =  (*set).get_current_transition();
                }
                else if(res->get_instance_type()  == resource_set<vk::render_texture>::get_class_type())
                {
                    eastl::shared_ptr< resource_set<vk::texture_2d> > set = eastl::static_pointer_cast< resource_set<vk::texture_2d>>(res);
                    last_trans = (*set).get_last_transition();
                    current_trans =  (*set).get_current_transition();
                }
                else if(res->get_instance_type()  == resource_set<vk::texture_cube>::get_class_type())
                {
                    eastl::shared_ptr< resource_set<vk::texture_2d> > set = eastl::static_pointer_cast< resource_set<vk::texture_2d>>(res);
                    last_trans = (*set).get_last_transition();
                    current_trans =  (*set).get_current_transition();
                }
                else
                {
                    
                    ++iter;
                    continue;
                }
                
                vk::usage_transition reset_trans = {};
                reset_trans.previous = current_trans.current;
                reset_trans.current = last_trans.current;
                
                EA_ASSERT(reset_trans.current != vk::image::image_layouts::UNDEFINED);
                
                eastl::shared_ptr<vk::image> p_image = eastl::static_pointer_cast<vk::image>(res);
                node_type::create_barrier(buffer, p_image.get(), d.node, image_id,reset_trans);
                ++iter;
            }
        }
        
        virtual void create_gpu_resources() override
        {}
        
        virtual bool record_node_commands(command_recorder& buffer, uint32_t image_id) override
        {
            return false;
        }
        
        virtual void init_node() override
        {}
        
        virtual void update_node(vk::camera& camera, uint32_t image_id) override
        {}
        
        virtual VkPipelineStageFlagBits get_producer_stage() override {return VK_PIPELINE_STAGE_FLAG_BITS_MAX_ENUM;};
        virtual VkPipelineStageFlagBits get_consumer_stage() override { return VK_PIPELINE_STAGE_FLAG_BITS_MAX_ENUM;};
        
        virtual bool  record(command_recorder& buffer, uint32_t image_id) override
        {
            node_type::record(buffer, image_id);
            
            return false;
        }
    private:
        
        texture_registry<NUM_CHILDREN> _texture_registry;
        command_recorder _commands;
        material_store& _material_store;
    };
}

