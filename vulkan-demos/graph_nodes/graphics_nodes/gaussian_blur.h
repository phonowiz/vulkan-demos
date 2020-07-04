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
    
    eastl::fixed_string<char, 20> _input_texture;
    eastl::fixed_string<char, 20> _output_texture;
    vk::screen_plane _screen_plane;
    
public:
    
    enum class DIRECTION
    {
        VERTICAL = 0,
        HORIZONTAL
    };
    
    
    using parent_type = vk::graphics_node<1, NUM_CHILDREN>;
    using render_pass_type = typename parent_type::render_pass_type;
    using subpass_type = typename parent_type::render_pass_type::subpass_s;
    using object_vector_type = typename parent_type::object_vector_type;
    using image_ptr = eastl::shared_ptr<vk::image>;
    using tex_registry_type = typename parent_type::tex_registry_type;
    using material_store_type = typename parent_type::material_store_type;
    using object_submask_type = typename parent_type::object_subpass_mask;
    
    gaussian_blur(vk::device* dev, float width, float height, DIRECTION dir,
                  const char* input_tex, const char* output_tex):
    parent_type(dev, width, height),
    _screen_plane(dev)
    {
        _dir = dir;
        _input_texture = input_tex;
        _output_texture = output_tex;
        parent_type::_name = "gauss_horizontal";
        if( _dir == DIRECTION::VERTICAL)
        {
            parent_type::_name = "gauss_vertical";
        }
    }
    
    virtual void init_node() override
    {
        render_pass_type &pass = parent_type::_node_render_pass;
        object_vector_type &obj_vec = parent_type::_obj_vector;
        tex_registry_type* _tex_registry = parent_type::_texture_registry;
        material_store_type* _mat_store = parent_type::_material_store;
        object_vector_type& _obj_vector = parent_type::_obj_vector;
        
        EA_ASSERT_MSG(!_input_texture.empty(), "texture to be blurred has not been set");
        
        _screen_plane.create();
        
        vk::attachment_group<1>& attach_group = pass.get_attachment_group();
        
        vk::resource_set<vk::render_texture>& gaussblur_tex =
                _tex_registry->get_write_render_texture_set(_output_texture.c_str(), this, vk::usage_type::INPUT_ATTACHMENT);
        attach_group.add_attachment(gaussblur_tex, glm::vec4(1.0f));
        
        gaussblur_tex.set_format(vk::image::formats::R32G32B32A32_SIGNED_FLOAT);
        gaussblur_tex.set_filter(vk::image::filter::NEAREST);
        gaussblur_tex.init();
        
        subpass_type& sub_p = pass.add_subpass(parent_type::_material_store, "gaussblur");
        
        sub_p.init_parameter("blurScale", vk::parameter_stage::FRAGMENT, 1.f, 1);
        sub_p.init_parameter("blurStrength", vk::parameter_stage::FRAGMENT, 1.f, 1);
        sub_p.init_parameter("blurDirection", vk::parameter_stage::FRAGMENT, int(_dir), 1 );
        
        
        vk::resource_set<vk::render_texture>& target = _tex_registry->get_read_render_texture_set(_input_texture.c_str(), this, vk::usage_type::COMBINED_IMAGE_SAMPLER);
        
        sub_p.set_image_sampler( target, "samplerColor", vk::parameter_stage::FRAGMENT, 0, vk::usage_type::COMBINED_IMAGE_SAMPLER);
        sub_p.add_output_attachment(_output_texture.c_str(), render_pass_type::write_channels::RGBA, true);
        
        pass.add_object(static_cast<vk::obj_shape*>(&_screen_plane));
        
    }
    
    virtual void destroy() override
    {
        parent_type::destroy();
        _screen_plane.destroy();
    }
    virtual void update_node(vk::camera&, uint32_t)
    {
        
    }
    
private:
    DIRECTION _dir = DIRECTION::VERTICAL;
};
