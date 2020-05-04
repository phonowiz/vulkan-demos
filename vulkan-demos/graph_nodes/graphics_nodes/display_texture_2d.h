//
//  display_texture.hpp
//  vulkan-demos
//
//  Created by Rafael Sabino on 4/10/20.
//  Copyright © 2020 Rafael Sabino. All rights reserved.
//

#pragma once


#include "graphics_node.h"
#include "material_store.h"
#include "texture_registry.h"
#include "glfw_swapchain.h"
#include "screen_plane.h"
#include "texture_2d.h"
#include "attachment_group.h"

template< uint32_t NUM_CHILDREN>
class display_texture_2d : public vk::graphics_node<1, NUM_CHILDREN>
{
public:
    
    using parent_type = vk::graphics_node<1, NUM_CHILDREN>;
    using render_pass_type = typename parent_type::render_pass_type;
    using subpass_type = typename parent_type::render_pass_type::subpass_s;
    using object_vector_type = typename parent_type::object_vector_type;
    using image_ptr = std::shared_ptr<vk::image>;
    using tex_registry_type = typename parent_type::tex_registry_type;
    
    
    display_texture_2d(vk::device* dev, vk::glfw_swapchain* swapchain, uint32_t width,uint32_t height):
    parent_type(dev, width, height),
    _screen_plane(dev)
    {
        _swapchain = swapchain;
    }
    
    
    void use_shader(const char* shader)
    {
        _shader = shader;
    }
    void show_texture(const char* texture)
    {
        _texture = texture;
    }
    
    virtual void init_node() override
    {
        assert(_texture != nullptr);
        
        render_pass_type &pass = parent_type::_node_render_pass;
        object_vector_type &obj_vec = parent_type::_obj_vector;
        tex_registry_type* _tex_registry = parent_type::_texture_registry;
        
        
        pass.get_attachment_group().add_attachment( _swapchain->present_textures, glm::vec4(0.0f));
        
        _screen_plane.create();
        subpass_type& sub_p = pass.add_subpass(parent_type::_material_store, _shader);
        
        vk::texture_2d& ptr = _tex_registry->get_loaded_texture(_texture, this, parent_type::_device, _texture);
        
        sub_p.set_image_sampler(ptr, "tex", vk::material_base::parameter_stage::FRAGMENT, 1,
                                       vk::visual_material::usage_type::COMBINED_IMAGE_SAMPLER);
        
        int binding = 0;
        sub_p.init_parameter("width", vk::visual_material::parameter_stage::VERTEX,
                             _swapchain->get_vk_swap_extent().width, binding);
        sub_p.init_parameter("height", vk::visual_material::parameter_stage::VERTEX,
                             _swapchain->get_vk_swap_extent().height, binding);
        
        sub_p.add_output_attachment(0);
        pass.add_object(_screen_plane);
        
    }
    
    virtual void update_node(vk::camera& camera, uint32_t image_id) override
    {

    }
    
    virtual void destroy() override
    {
        _screen_plane.destroy();
        parent_type::destroy();
    }
    
private:
    
    using parent_type::add_object;
    
    const char* _shader = "display";
    vk::screen_plane _screen_plane;
    vk::glfw_swapchain* _swapchain = nullptr;
    const char* _texture = nullptr;
};


