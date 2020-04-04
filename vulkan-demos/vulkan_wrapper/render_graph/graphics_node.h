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
#include <assert.h>

namespace vk {

    template< uint32_t NUM_ATTACHMENTS, uint32_t NUM_CHILDREN =10u>
    class graphics_node : public vk::node<NUM_CHILDREN>
    {
    
    public:
        
        static constexpr uint32_t ALL_SUBPASSES = 0;
        
        using node_type = vk::node<NUM_CHILDREN>;
        using render_pass_type = render_pass<NUM_ATTACHMENTS>;
        using object_subpass_mask = eastl::fixed_map<vk::obj_shape*, uint32_t, 20, true>;
        using object_vector_type = eastl::fixed_vector<vk::obj_shape*, 20, true>;
        
        graphics_node(device* dev, uint32_t width, uint32_t height):
        _node_render_pass(dev, glm::vec2(width, height)),
        node_type::node_type(dev)
        {}
        
        virtual void record_node_commands(command_recorder& buffer, uint32_t image_id) override
        {
            _node_render_pass.set_clear_attachments_colors(glm::vec4(0.f));
            _node_render_pass.set_clear_depth(glm::vec2(1.0f, 0.0f));
            
            _node_render_pass.record_draw_commands(buffer.get_raw_graphics_command(image_id), image_id);
        }
        
        virtual void destroy() override
        {
            _node_render_pass.destroy();
        }
        
        void add_object(vk::object& object, uint32_t subpass_id = ALL_SUBPASSES)
        {
            uint32_t new_mask = 1 << subpass_id;
            uint32_t old_mask = _obj_subpass_mask[&object];
            
            uint32_t combine = new_mask | old_mask;
            
            _obj_subpass_mask[object] = combine;
            
            _obj_vector.push_back(&object);
        }
        
    protected:
        
        bool subpass_exclude(vk::obj_shape* obj, uint32_t subpass_id)
        {
            assert(_obj_subpass_mask.find(obj) != _obj_subpass_mask.end() );
            
            uint32_t subpass_mask = _obj_subpass_mask[obj];
            bool result = ALL_SUBPASSES == subpass_mask;
            if(!result)
            {
                result = static_cast<bool>( subpass_mask | 1 << subpass_id );
            }
            
            return result;
                
        }
        virtual void create_gpu_resources() override
        {
            for( uint32_t i = 0; i < vk::glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++i)
            {
                _node_render_pass.create(i);
            }
        }
        
    protected:
        
        render_pass_type _node_render_pass;
        object_subpass_mask  _obj_subpass_mask;
        object_vector_type  _obj_vector;
    };
}
