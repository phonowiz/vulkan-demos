//
//  clear_3d_texture.cpp
//  vulkan-demos
//
//  Created by Rafael Sabino on 4/17/20.
//  Copyright © 2020 Rafael Sabino. All rights reserved.
//

#pragma once

#include "compute_node.h"
#include "texture_registry.h"
#include "texture_3d.h"

template< uint32_t NUM_CHILDREN>
class clear_3d_textures: public vk::compute_node<NUM_CHILDREN>
{
public:
    using parent_type = vk::compute_node<NUM_CHILDREN>;
    using tex_registry_type = typename parent_type::tex_registry_type;
    using material_store_type = typename vk::node<NUM_CHILDREN>::material_store_type;
    using compute_pipeline_type = typename parent_type::compute_pipeline_type;
    
    
    clear_3d_textures(){}
    
    clear_3d_textures(vk::device* dev, eastl::fixed_string< char, 100 >& input_textures,
                       uint32_t group_width, uint32_t group_height, uint32_t group_depth =1):
    parent_type(dev, group_width, group_height, group_depth)
    {
        _albedo_texture = input_textures;
        
    }
    
    void set_clear_texture(eastl::fixed_string< char, 100 >&  input_tex, eastl::fixed_string< char, 100 >& normal_texture)
    {
        _albedo_texture = input_tex;
        _normal_texture = normal_texture;
    }
    
    virtual void update_node(vk::camera& camera, uint32_t image_id) override
    {
    }
    
    virtual void init_node() override
    {
        tex_registry_type* _tex_registry = parent_type::_texture_registry;
        material_store_type* _mat_store = parent_type::_material_store;
        compute_pipeline_type& _compute_pipelines = parent_type::_compute_pipelines;
        
        parent_type::_compute_pipelines.set_material("clear_3d_texture", *_mat_store);
        
        vk::resource_set<vk::texture_3d>& albedo_tx =
            _tex_registry->get_write_texture_3d_set(_albedo_texture.c_str(), this);
        
        vk::resource_set<vk::texture_3d>& normal_tx =
        _tex_registry->get_write_texture_3d_set(_normal_texture.c_str(), this);
        

        uint32_t size =  parent_type::_group_x * vk::compute_pipeline<1>::LOCAL_GROUP_SIZE;
        
        albedo_tx.set_device(parent_type::_device);
        albedo_tx.set_dimensions(size, size, size);
        albedo_tx.set_filter(vk::image::filter::LINEAR);
        //albedo_tx.set_format(vk::image::formats::R32G32B32A32_SIGNED_FLOAT);
        
        normal_tx.set_device(parent_type::_device);
        normal_tx.set_dimensions(size, size, size);
        normal_tx.set_filter(vk::image::filter::LINEAR);
        //normal_tx.set_format(vk::image::formats::R32G32B32A32_SIGNED_FLOAT);
        
        albedo_tx.init();
        normal_tx.init();

        //TODO: ADAPT THIS SHADER TO TAKE IN TWO TEXTURES SO THAT WE CAN CLEAR NORMAL TEXTURES AS WELL
        parent_type::_compute_pipelines.set_image_sampler( albedo_tx, "texture_3d", 0);
        
    }
    
private:
    eastl::fixed_string< char, 100 > _albedo_texture = {};
    eastl::fixed_string< char, 100 > _normal_texture = {};
};


template class clear_3d_textures<1>;
