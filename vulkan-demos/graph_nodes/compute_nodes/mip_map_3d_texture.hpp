//
//  mip_map_3d_texture.hpp
//  vulkan-demos
//
//  Created by Rafael Sabino on 4/13/20.
//  Copyright © 2020 Rafael Sabino. All rights reserved.
//

#pragma once

#include "compute_node.h"
#include "texture_registry.h"
#include "texture_3d.h"


template<uint32_t NUM_CHILDREN>
class mip_map_3d_texture : public vk::compute_node<NUM_CHILDREN>
{
public:
    using parent_type = vk::compute_node<NUM_CHILDREN>;
    using tex_registry_type = typename parent_type::tex_registry_type;
    using material_store_type = typename vk::node<NUM_CHILDREN>::material_store_type;
    
    mip_map_3d_texture(vk::device* dev, const char* input_texture, const char* output_texture,
                       uint32_t group_width, uint32_t group_height, uint32_t group_depth =1):
    parent_type(dev, group_width, group_height, group_depth)
    {
        _input_texture = input_texture;
        _output_texture = output_texture;
        
        assert(input_texture);
        assert(output_texture);
    }
    
    virtual void init_node() override
    {
        tex_registry_type* _tex_registry = parent_type::_texture_registry;
        material_store_type* _mat_store = parent_type::_material_store;
        
        for( int i = 0; i < vk::NUM_SWAPCHAIN_IMAGES; ++i)
        {
            parent_type::_compute_pipelines.set_material("downsize", *_mat_store);
        }
        
//        static const char* albedo_names[6] = {"voxel_albedos", "voxel_albedos1", "voxel_albedos2", "voxel_albedos3", "voxel_albedos4", "voxel_albedos5"};
//        static const char* normal_names[6] = {"voxel_normals", "voxel_normals1","voxel_normals2", "voxel_normals3", "voxel_normals4", "voxel_normals5"};
        
        vk::resource_set<vk::texture_3d>& text = _tex_registry->get_read_texture_3d_set(_input_texture, this,
                                                                               vk::image::image_layouts::SHADER_READ_ONLY_OPTIMAL);
        
        
        
    }
    
private:
    const char* _input_texture = nullptr;
    const char* _output_texture = nullptr;
};



template class mip_map_3d_texture<1>;
