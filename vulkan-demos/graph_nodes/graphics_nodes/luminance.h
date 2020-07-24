//
//  luminance.h
//  vulkan-demos
//
//  Created by Rafael Sabino on 7/22/20.
//  Copyright © 2020 Rafael Sabino. All rights reserved.
//

#pragma once

#include "graphics_node.h"

static const uint32_t LUMINANCE_ATTACHMENTS = 1;

template< uint32_t NUM_CHILDREN>
class luminance : public vk::graphics_node<LUMINANCE_ATTACHMENTS, NUM_CHILDREN>
{
private:
    vk::screen_plane _screen_plane;
    vk::glfw_swapchain* _swapchain = nullptr;
public:
    
    using parent_type = vk::graphics_node<LUMINANCE_ATTACHMENTS, NUM_CHILDREN>;
    using render_pass_type = typename parent_type::render_pass_type;
    using subpass_type = typename parent_type::render_pass_type::subpass_s;
    using object_vector_type = typename parent_type::object_vector_type;
    using image_ptr = eastl::shared_ptr<vk::image>;
    using tex_registry_type = typename parent_type::tex_registry_type;
    using material_store_type = typename parent_type::material_store_type;
    using object_submask_type = typename parent_type::object_subpass_mask;
    

    luminance(vk::device* dev, vk::glfw_swapchain* swapchain):
    parent_type(dev,swapchain->get_vk_swap_extent().width ,swapchain->get_vk_swap_extent().height),
    _screen_plane(dev), _swapchain(swapchain)
    {}
    
    virtual void init_node() override
    {
        render_pass_type &pass = parent_type::_node_render_pass;
        object_vector_type &obj_vec = parent_type::_obj_vector;
        tex_registry_type* _tex_registry = parent_type::_texture_registry;
        material_store_type* _mat_store = parent_type::_material_store;
        object_vector_type& _obj_vector = parent_type::_obj_vector;
       
        _screen_plane.create();
        
        subpass_type& luminance_subpass = pass.add_subpass(_mat_store, "luminance");
        
        vk::resource_set<vk::render_texture>& final_render =  _tex_registry->get_read_render_texture_set("final_render", this, vk::usage_type::INPUT_ATTACHMENT);
        vk::resource_set<vk::render_texture>& luminance = _tex_registry->get_write_render_texture_set("luminance", this);

        vk::attachment_group<LUMINANCE_ATTACHMENTS>& luminance_attachment_grp = pass.get_attachment_group();
       
        luminance_attachment_grp.add_attachment( luminance, glm::vec4(0.0f));
       

        luminance.set_filter(vk::image::filter::NEAREST);
        luminance.set_format(vk::image::formats::R32_SIGNED_FLOAT);
        luminance.init();
        
        luminance_subpass.set_image_sampler(final_render, "color", vk::parameter_stage::FRAGMENT, 0, vk::usage_type::COMBINED_IMAGE_SAMPLER);
        luminance_subpass.add_output_attachment("luminance");
        
        pass.add_object(static_cast<vk::obj_shape*>(&_screen_plane));

    }
    
    virtual void update_node(vk::camera& camera, uint32_t image_id) override
    {
    
    }
    
    virtual void destroy() override
    {
        _screen_plane.destroy();
        parent_type::destroy();
    }
    
};

template class luminance<1>;
