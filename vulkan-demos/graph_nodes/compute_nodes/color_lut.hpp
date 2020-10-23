//
//  lut.hpp
//  vulkan-demos
//
//  Created by Rafael Sabino on 10/16/20.
//  Copyright © 2020 Rafael Sabino. All rights reserved.
//

#pragma once

#include "compute_node.h"
#include "texture_registry.h"
#include "texture_3d.h"
#include "voxelize.h"

template< uint32_t NUM_CHILDREN>
class color_lut: public vk::compute_node<NUM_CHILDREN>
{
private:
    int _count = 0;
    
    static const int32_t LUT_SIZE = 32;
public:
    using parent_type = vk::compute_node<NUM_CHILDREN>;
    using tex_registry_type = typename parent_type::tex_registry_type;
    using material_store_type = typename vk::node<NUM_CHILDREN>::material_store_type;
    using compute_pipeline_type = typename parent_type::compute_pipeline_type;
 
    color_lut( vk::device* dev,
              uint32_t group_width, uint32_t group_height, uint32_t group_depth =1):
    parent_type(dev, group_width, group_height, group_depth)
    {
    }
    
    virtual void init_node() override
    {

        tex_registry_type* _tex_registry = parent_type::_texture_registry;
        material_store_type* _mat_store = parent_type::_material_store;
        compute_pipeline_type& _compute_pipelines = parent_type::_compute_pipelines;
        
        parent_type::_compute_pipelines.set_material("color_lut", *_mat_store);
                vk::resource_set<vk::texture_3d>& color_lut = _tex_registry->get_write_texture_3d_set("color_lut", this);
        
        color_lut.set_filter(vk::image::filter::LINEAR);
        color_lut.set_dimensions(LUT_SIZE, LUT_SIZE,LUT_SIZE);
        color_lut.set_format(vk::image::formats::R16G16B16A16_UNSIGNED_NORMALIZED);
        color_lut.init();
        
        _compute_pipelines.set_image_sampler(color_lut, "lut", 0);
        
        _compute_pipelines.init_parameter("width", static_cast<float>(LUT_SIZE), 1);
        _compute_pipelines.init_parameter("height", static_cast<float>(LUT_SIZE), 1);
        _compute_pipelines.init_parameter("depth", static_cast<float>(LUT_SIZE), 1);
        
    }
    
    
    virtual void update_node(vk::camera& camera, uint32_t image_id) override
    {
    }
    
    virtual bool record_node_commands(vk::command_recorder& buffer, uint32_t image_id) override
    {
        if(_count < vk::NUM_SWAPCHAIN_IMAGES)
        {
            parent_type::record_node_commands(buffer, image_id);
            ++_count;
        }
        return true;
    }
};

template class color_lut<1>;
