//
//  graphics_node.hpp
//  vulkan-demos
//
//  Created by Rafael Sabino on 1/18/20.
//  Copyright © 2020 Rafael Sabino. All rights reserved.
//

#pragma once

#include "node.h"
#include "render_pass.h"
#include "EASTL/fixed_vector.h"
#include "texture_registry.h"
#include "object.h"
#include "assimp_node.h"
#include <assert.h>

namespace vk {

    template< uint32_t NUM_ATTACHMENTS, uint32_t NUM_CHILDREN =10u>
    class graphics_node : public vk::node<NUM_CHILDREN>
    {
    
    public:
        
        static constexpr uint32_t ALL_SUBPASSES = 0;
        
        using node_type = vk::node<NUM_CHILDREN>;
        using render_pass_type =  render_pass<NUM_ATTACHMENTS>;
        using mesh_node = vk::assimp_node<NUM_CHILDREN>;
        using object_subpass_mask = eastl::fixed_map<mesh_node*, uint32_t, 20, true>;
        using object_vector_type = eastl::fixed_vector<vk::assimp_node<NUM_CHILDREN>*, 20, true>;
        
        graphics_node(){}
        
        graphics_node(device* dev, uint32_t width, uint32_t height):
        _node_render_pass(dev, glm::vec2(width, height)),
        node_type::node_type(dev)
        {}
        
        virtual bool record_node_commands(command_recorder& buffer, uint32_t image_id) override
        {
            _node_render_pass.commit_parameters_to_gpu(image_id);
            static constexpr uint32_t instance_count = 1;
            _node_render_pass.record_draw_commands(buffer.get_raw_graphics_command(image_id), image_id, instance_count);
            
            return true;
        }
        
        inline void set_dimensions( uint32_t width, uint32_t height)
        {
            _node_render_pass.set_dimensions(glm::vec2(width, height));
        }
        
        virtual void init() override
        {
            assert(node_type::_device != nullptr);
            _node_render_pass.set_device(node_type::_device);
            
            node_type::init();
        }
        virtual void destroy() override
        {
            _node_render_pass.destroy();
        }
        
        virtual void add_child( node_type& child ) override
        {
            node_type::add_child(child);
            
            if( child.get_instance_type() == vk::assimp_node<NUM_CHILDREN>::get_class_type())
            {
                add_object(static_cast<mesh_node*>(&child));
            }
        }
        
        
        VkPipelineStageFlagBits get_producer_stage() override {  return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; };
        VkPipelineStageFlagBits get_consumer_stage() override {  return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT; };
        
    protected:
        
        void add_object(mesh_node* object)
        {
            _obj_vector.push_back(object);
        }
        
        
        bool set_dynamic_param(const char* name, uint32_t image_id,
                                uint32_t subpass_id, obj_shape* obj,  glm::mat4 mat, uint32_t binding)
        {
            typename render_pass_type::subpass_s& subpass = _node_render_pass.get_subpass(subpass_id);
            
            //TODO: This is slow. This will not be a factor in the near futre since we don't have that many
            //objects.  Possible solution?: eastl::fixed_map.
            
            int32_t count = 0;
            int i = 0;
            
            bool result = false;
            while(i < _node_render_pass.get_num_objs())
            {
                if(!subpass.is_ignored(i))
                {
                    if(obj == _node_render_pass.get_object(i))
                    {
                        //use the index to access the dynamic parameter memory for this object
                        subpass.get_pipeline(image_id).
                                            get_dynamic_parameters(parameter_stage::VERTEX, binding)[count][name] = mat;
                        
                        result = true;
                        break;
                    }
                    ++count;
                }
                ++i;
            }
            EA_ASSERT_MSG(result != false, "you are trying to set a dynamic parameter to an object not included in this subpass");
            return result;
        }
        
        void add_dynamic_param(const char* name, uint32_t subpass_id,
                               parameter_stage stage, glm::mat4 mat, uint32_t binding)
        {
            int count = 0;
            typename render_pass_type::subpass_s& subpass = _node_render_pass.get_subpass(subpass_id);
            for( int i = 0; i < _node_render_pass.get_num_objs(); ++i)
            {
                if(!subpass.is_ignored(i))
                {
                    ++count;
                }
            }
            EA_ASSERT_MSG(count != 0, "dynamic parameters cannot be created without adding objects to this subpass");
            subpass.init_dynamic_params(name,
                                        parameter_stage::VERTEX, mat, count, binding);

        }
        
        virtual void create_gpu_resources() override
        {
            _node_render_pass.init_attachment_group();
            for( uint32_t i = 0; i < vk::glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++i)
            {
                _node_render_pass.create(i);
            }
        }
        
        virtual char const * const * get_instance_type() override { return (&_node_type); };
        static char const * const *  get_class_type(){ return (&_node_type); }
        
    private:
        
        static constexpr char const * _node_type = nullptr;
        
    protected:
        
        eastl::fixed_vector<mesh_node, 10> _meshes;
        
        render_pass_type _node_render_pass;
        object_vector_type  _obj_vector;
        
        
    };
}
