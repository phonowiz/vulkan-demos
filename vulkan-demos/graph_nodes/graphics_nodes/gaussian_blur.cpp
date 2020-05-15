//
//  gaussian_blur.cpp
//  vulkan-demos
//
//  Created by Rafael Sabino on 5/13/20.
//  Copyright © 2020 Rafael Sabino. All rights reserved.
//

#pragma once

#include "EAAssert/eaassert.h"
#include "graphics_node.h"
#include "screen_plane.h"


template< uint32_t NUM_CHILDREN>
class gaussian_blur : public vk::graphics_node<1, NUM_CHILDREN>
{
    
private:
    
    eastl::fixed_string<char, 20> _texture;
    vk::screen_plane _screen_plane;
    
public:
    
    using parent_type = vk::graphics_node<1, NUM_CHILDREN>;
    using render_pass_type = typename parent_type::render_pass_type;
    using subpass_type = typename parent_type::render_pass_type::subpass_s;
    using object_vector_type = typename parent_type::object_vector_type;
    using image_ptr = eastl::shared_ptr<vk::image>;
    using tex_registry_type = typename parent_type::tex_registry_type;
    using material_store_type = typename parent_type::material_store_type;
    using object_submask_type = typename parent_type::object_subpass_mask;
    
    gaussian_blur(vk::device* dev, float width, float height,
                  const char* tex):
    parent_type(dev, width, height),
    _screen_plane(dev)
    {
        _texture = tex;
    }
    
    void set_texture( eastl::fixed_string<char, 20>& name)
    {
        _texture = name;
    }
    
    virtual void init_node() override
    {
        render_pass_type &pass = parent_type::_node_render_pass;
        object_vector_type &obj_vec = parent_type::_obj_vector;
        tex_registry_type* _tex_registry = parent_type::_texture_registry;
        material_store_type* _mat_store = parent_type::_material_store;
        object_submask_type& _obj_masks = parent_type::_obj_subpass_mask;
        object_vector_type& _obj_vector = parent_type::_obj_vector;
        
        EA_ASSERT_MSG(!_texture.empty(), "texture to be blurred has not been set");
        
        _screen_plane.create();
        
        vk::attachment_group<1>& attach_group = pass.get_attachment_group();
        
        vk::resource_set<vk::render_texture>& gaussblur_tex = _tex_registry->get_write_render_texture_set("gaussblur", this, vk::usage_type::INPUT_ATTACHMENT);
        attach_group.add_attachment(gaussblur_tex, glm::vec4(0.0f));
        
        gaussblur_tex.init();
        
        //TODO: graph needs to be figured out by the graph, remove once implemented
        gaussblur_tex.set_native_layout(vk::image::image_layouts::SHADER_READ_ONLY_OPTIMAL);
        subpass_type& sub_p = pass.add_subpass(parent_type::_material_store, "gaussblur");
        
        sub_p.init_parameter("blurScale", vk::parameter_stage::FRAGMENT, 1.0f, 1);
        sub_p.init_parameter("blurStrength", vk::parameter_stage::FRAGMENT, 1.5f, 1);
        sub_p.init_parameter("blurDirection", vk::parameter_stage::FRAGMENT, int(1), 1 );
        
        vk::resource_set<vk::render_texture>& target = _tex_registry->get_read_render_texture_set(_texture.c_str(), this, vk::usage_type::COMBINED_IMAGE_SAMPLER);
        
        sub_p.set_image_sampler( target, "samplerColor", vk::parameter_stage::FRAGMENT, 0, vk::usage_type::COMBINED_IMAGE_SAMPLER);
        sub_p.add_output_attachment("gaussblur", render_pass_type::write_channels::RGBA, true);
        
        pass.add_object(_screen_plane);
        
    }
    
    virtual void update_node(vk::camera&, uint32_t)
    {
        
    }
    
};
