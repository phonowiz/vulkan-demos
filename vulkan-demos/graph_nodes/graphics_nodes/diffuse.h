//
//  diffuse.hpp
//  vulkan-demos
//
//  Created by Rafael Sabino on 6/19/20.
//  Copyright © 2020 Rafael Sabino. All rights reserved.
//

#pragma once

template< uint32_t NUM_CHILDREN>
class diffuse : public vk::graphics_node<1, NUM_CHILDREN>
{
public:
    
    using parent_type = vk::graphics_node<1, NUM_CHILDREN>;
    using render_pass_type = typename parent_type::render_pass_type;
    using subpass_type = typename parent_type::render_pass_type::subpass_s;
    using object_vector_type = typename parent_type::object_vector_type;
    using image_ptr = eastl::shared_ptr<vk::image>;
    using tex_registry_type = typename parent_type::tex_registry_type;
    
    
    
    diffuse(vk::device* dev, uint32_t width, uint32_t height):
    parent_type(dev, width, height)
    {
        
    }
    
    virtual void init_node() override
    {
        
    }
    
    virtual void update_node(vk::camera& camera, uint32_t image_id) override
    {
    }
    
    virtual void destroy() override
    {
        parent_type::destroy();
    }
    
private:
    
};
