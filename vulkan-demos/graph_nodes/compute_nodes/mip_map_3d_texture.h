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
    
    mip_map_3d_texture(vk::device* dev, uint32_t group_width, uint32_t group_height, uint32_t group_depth =1):
    parent_type(dev, group_width, group_height, group_depth)
    {
        
    }
    
    virtual void init_node() override
    {
        tex_registry_type* _tex_registry = parent_type::_texture_registry;
        
        std::shared_ptr<vk::texture_3d> text = _tex_registry->get_read_texture("name", this,
                                                                               vk::image::image_layouts::SHADER_READ_ONLY_OPTIMAL);
        
        
    }
};

