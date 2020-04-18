//
//  mrt.cpp
//  vulkan-demos
//
//  Created by Rafael Sabino on 4/17/20.
//  Copyright © 2020 Rafael Sabino. All rights reserved.
//

#pragma once

#include "graphics_node.h"
#include "material_store.h"
#include "texture_registry.h"



template<uint32_t NUM_CHILDREN>
class mrt: public vk::graphics_node<4, NUM_CHILDREN>
{
public:
    
    using parent_type = vk::graphics_node<4, NUM_CHILDREN>;
    using render_pass_type = typename parent_type::render_pass_type;
    using subpass_type = typename parent_type::render_pass_type::subpass_s;
    using object_vector_type = typename parent_type::object_vector_type;
    using image_ptr = std::shared_ptr<vk::image>;
    using tex_registry_type = typename parent_type::tex_registry_type;
    using material_store_type = typename parent_type::material_store_type;
    using object_submask_type = typename parent_type::object_subpass_mask;
    
    mrt(vk::device* dev, vk::glfw_swapchain* swapchain, glm::vec2 _render_dims):
    parent_type(dev, _render_dims.x, _render_dims.y),
    _screen_plane(dev)
    {
        _swapchain= swapchain;
        _screen_plane.create();
    }
    
    virtual void init_node() override
    {
        render_pass_type &pass = parent_type::_node_render_pass;
        object_vector_type &obj_vec = parent_type::_obj_vector;
        tex_registry_type* _tex_registry = parent_type::_texture_registry;
        material_store_type* _mat_store = parent_type::_material_store;
        object_submask_type& _obj_masks = parent_type::_obj_subpass_mask;
        object_vector_type& _obj_vector = parent_type::_obj_vector;
        
        subpass_type& mrt_subpass = pass.add_subpass(_mat_store, "mrt");
        subpass_type& subpass = pass.add_subpass(_mat_store, "deferred_output");
        
        pass.add_object(_screen_plane);
        pass.skip_subpass(_screen_plane, 0);
        
        for(int i = 0; i < _obj_vector.size(); ++i)
        {
            pass.add_object(*_obj_vector[i]);
            pass.skip_subpass( *_obj_vector[i], 1);
        }
        
        vk::attachment_group<4>& mrt_attachment_group = pass.get_attachment_group();
        
        vk::resource_set<vk::render_texture>& normals = _tex_registry->get_write_render_texture_set("normals", this);
        vk::resource_set<vk::render_texture>& albedos = _tex_registry->get_write_render_texture_set("albedos", this);
        
        //TODO: you can derive positon from depth and sampling fragment position
        vk::resource_set<vk::render_texture>& positions = _tex_registry->get_write_render_texture_set("positions", this);
        vk::resource_set<vk::depth_texture>& depth = _tex_registry->get_write_depth_texture_set("depth", this);
        
        mrt_attachment_group.add_attachment(depth);
        
        mrt_attachment_group.add_attachment(normals);
        mrt_attachment_group.add_attachment(albedos);
        mrt_attachment_group.add_attachment(positions);
        
    }
    
    
private:
    vk::screen_plane _screen_plane;
    vk::glfw_swapchain* _swapchain = nullptr;
};

template class mrt<4>;
