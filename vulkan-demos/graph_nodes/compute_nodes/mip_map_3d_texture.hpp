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
    
    static constexpr unsigned int TOTAL_LODS = 6;
    
    using parent_type = vk::compute_node<NUM_CHILDREN>;
    using tex_registry_type = typename parent_type::tex_registry_type;
    using material_store_type = typename vk::node<NUM_CHILDREN>::material_store_type;
    using compute_pipeline_type = typename parent_type::compute_pipeline_type;
    
    mip_map_3d_texture(){}
    
    mip_map_3d_texture(vk::device* dev, eastl::array< eastl::fixed_string<char, 100>, 2>& input_textures,
                       eastl::array< eastl::fixed_string<char, 100>, 2>& output_textures, uint32_t group_width, uint32_t group_height, uint32_t group_depth =1):
    parent_type(dev, group_width, group_height, group_depth)
    {
        _input_textures = input_textures;
        _output_textures = output_textures;
    }
    
    
    void set_textures( eastl::array< eastl::fixed_string<char, 100>, 2>& input_textures,
                        eastl::array< eastl::fixed_string<char, 100>, 2>& output_textures )
    {
        _input_textures = input_textures;
        _output_textures = output_textures;
    }
    
    virtual void init_node() override
    {
        assert( !_input_textures[0].empty() && !_input_textures[1].empty() &&"you need 2 input textures");
        assert( !_output_textures[0].empty() && !_output_textures[1].empty() && "you need 2 output textures");
        
        tex_registry_type* _tex_registry = parent_type::_texture_registry;
        material_store_type* _mat_store = parent_type::_material_store;
        compute_pipeline_type& _compute_pipelines = parent_type::_compute_pipelines;
        
        parent_type::_compute_pipelines.set_material("downsize", *_mat_store);
        
        
        vk::resource_set<vk::texture_3d>& input_tex1 = _tex_registry->get_read_texture_3d_set(_input_textures[0].c_str(), this,
                                                                               vk::usage_type::STORAGE_IMAGE);
        vk::resource_set<vk::texture_3d>& input_tex2 = _tex_registry->get_read_texture_3d_set(_input_textures[1].c_str(), this,
                                                                               vk::usage_type::STORAGE_IMAGE);
        
        assert(_tex_registry->is_resource_created(_output_textures[0].c_str()) && "output resource hasn't been created");
        
        vk::resource_set<vk::texture_3d>& out_tex1 = _tex_registry->get_write_texture_3d_set(_output_textures[0].c_str(), this);
        vk::resource_set<vk::texture_3d>& out_tex2 = _tex_registry->get_write_texture_3d_set(_output_textures[1].c_str(), this);
        
        
        _compute_pipelines.set_image_sampler(input_tex1, "r_texture_1", 0);
        _compute_pipelines.set_image_sampler(input_tex2, "r_texture_2", 1);
        _compute_pipelines.set_image_sampler(out_tex1, "w_texture_1", 2);
        _compute_pipelines.set_image_sampler(out_tex2, "w_texture_2", 3);
    }

    virtual void update_node(vk::camera& camera, uint32_t image_id) override
    {
        
    }
    virtual void destroy() override
    {
        vk::compute_node<NUM_CHILDREN>::destroy();
    }
    
private:
    eastl::array< eastl::fixed_string<char, 100>, 2> _input_textures = {};
    eastl::array< eastl::fixed_string<char, 100>, 2> _output_textures = {};
};



template class mip_map_3d_texture<1>;
