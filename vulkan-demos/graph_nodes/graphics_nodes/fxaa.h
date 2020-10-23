//
//  fxaa.h
//  vulkan-demos
//
//  Created by Rafael Sabino on 7/21/20.
//  Copyright © 2020 Rafael Sabino. All rights reserved.
//

#pragma once

#include "graphics_node.h"


//implemenation based off of https://catlikecoding.com/unity/tutorials/advanced-rendering/fxaa/

static const uint32_t FXAA_ATTACHMENTS = 1;
template< uint32_t NUM_CHILDREN>
class fxaa : public vk::graphics_node<FXAA_ATTACHMENTS, NUM_CHILDREN>
{
private:
    vk::screen_plane _screen_plane;
    vk::glfw_swapchain* _swapchain = nullptr;
    
    const char* _aliased_texture =nullptr;
public:
    
    using parent_type = vk::graphics_node<FXAA_ATTACHMENTS, NUM_CHILDREN>;
    using render_pass_type = typename parent_type::render_pass_type;
    using subpass_type = typename parent_type::render_pass_type::subpass_s;
    using object_vector_type = typename parent_type::object_vector_type;
    using image_ptr = eastl::shared_ptr<vk::image>;
    using tex_registry_type = typename parent_type::tex_registry_type;
    using material_store_type = typename parent_type::material_store_type;
    using object_submask_type = typename parent_type::object_subpass_mask;
    
    
    fxaa(vk::device* dev, vk::glfw_swapchain* swapchain, const char* aliased_texture):
    parent_type(dev,swapchain->get_vk_swap_extent().width ,swapchain->get_vk_swap_extent().height),
    _screen_plane(dev), _swapchain(swapchain),_aliased_texture(aliased_texture)
    {}

    virtual void init_node() override
    {
        render_pass_type &pass = parent_type::_node_render_pass;
        object_vector_type &obj_vec = parent_type::_obj_vector;
        tex_registry_type* _tex_registry = parent_type::_texture_registry;
        material_store_type* _mat_store = parent_type::_material_store;
        object_vector_type& _obj_vector = parent_type::_obj_vector;
       
        _screen_plane.create();
        
        subpass_type& fxaa_subpass = pass.add_subpass(_mat_store, "fxaa");
        
        vk::resource_set<vk::render_texture>& aliased_tex =  _tex_registry->get_read_render_texture_set(_aliased_texture, this, vk::usage_type::COMBINED_IMAGE_SAMPLER);
        
        vk::attachment_group<FXAA_ATTACHMENTS>& fxaa_attachment_grp = pass.get_attachment_group();
       
        fxaa_attachment_grp.add_attachment( _swapchain->present_textures, glm::vec4(0.0f));
       

        fxaa_subpass.set_image_sampler(aliased_tex, "final_render", vk::parameter_stage::FRAGMENT, 0);
        
        fxaa_subpass.add_output_attachment("present");
        
        glm::vec4 texel_size = glm::vec4(0.0f);
        texel_size.x = 1.0f/_swapchain->get_vk_swap_extent().width;
        texel_size.y = 1.0f/_swapchain->get_vk_swap_extent().height;
        
        fxaa_subpass.init_parameter("maintex_texel_size", vk::parameter_stage::FRAGMENT, texel_size, 1);
        fxaa_subpass.init_parameter("contrast_threshold", vk::parameter_stage::FRAGMENT, .0833f, 1);
        fxaa_subpass.init_parameter("relative_threshold", vk::parameter_stage::FRAGMENT, 0.125f, 1);
        fxaa_subpass.init_parameter("subpixel_blending", vk::parameter_stage::FRAGMENT, 1.00f, 1);
        
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

template class fxaa<1>;
