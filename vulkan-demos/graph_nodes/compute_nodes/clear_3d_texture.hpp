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
    clear_3d_textures(vk::device* dev, eastl::array<const char*, 2>& input_textures,
                       uint32_t group_width, uint32_t group_height, uint32_t group_depth =1):
    parent_type(dev, group_width, group_height, group_depth)
    {
        _input_textures = input_textures;
        
    }
    
    virtual void update(vk::camera& camera, uint32_t image_id) override
    {
        
    }
    
    virtual void init_node() override
    {
        tex_registry_type* _tex_registry = parent_type::_texture_registry;
        material_store_type* _mat_store = parent_type::_material_store;
        compute_pipeline_type& _compute_pipelines = parent_type::_compute_pipelines;
        
        parent_type::_compute_pipelines.set_material("clear_3d_texture", *_mat_store);
        
        vk::resource_set<vk::texture_3d>& three_d_texture =
                    _tex_registry->get_write_texture_3d_set(_input_textures[0], this);
        
        parent_type::_compute_pipelines.set_image_sampler( three_d_texture, "texture_3d", 0);
        
    }
    
private:
    eastl::array<const char*, 2> _input_textures = {};
};


template class clear_3d_textures<1>;
