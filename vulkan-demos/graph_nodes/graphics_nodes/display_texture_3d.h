//
//  display_texture_3d.cpp
//  vulkan-demos
//
//  Created by Rafael Sabino on 4/19/20.
//  Copyright © 2020 Rafael Sabino. All rights reserved.
//

#pragma once

#include "graphics_node.h"
#include "material_store.h"
#include "texture_registry.h"
#include "glfw_swapchain.h"
#include "screen_plane.h"
#include "texture_3d.h"
#include "attachment_group.h"
#include "perspective_camera.h"


template< uint32_t NUM_CHILDREN>
class display_texture_3d : public vk::graphics_node<1, NUM_CHILDREN>
{
public:
    
    using parent_type = vk::graphics_node<1, NUM_CHILDREN>;
    using render_pass_type = typename parent_type::render_pass_type;
    using subpass_type = typename parent_type::render_pass_type::subpass_s;
    using object_vector_type = typename parent_type::object_vector_type;
    using image_ptr = eastl::shared_ptr<vk::image>;
    using tex_registry_type = typename parent_type::tex_registry_type;
    
    
    display_texture_3d(vk::device* dev, vk::glfw_swapchain* swapchain, glm::vec2 dims, const char* texture):
    parent_type(dev, dims.x, dims.y),
    _cube(dev, "cube.obj")
    {
        _swapchain = swapchain;
        _texture = texture;
    }
    
    void set_3D_texture_cam(vk::perspective_camera& cam)
    {
        _three_d_cam = &cam;
    }
    
    virtual void init_node() override
    {
        EA_ASSERT_MSG(_texture != nullptr, "No 3D texture has been specifed to be displayed");
        
        render_pass_type &pass = parent_type::_node_render_pass;
        object_vector_type &obj_vec = parent_type::_obj_vector;
        tex_registry_type* _tex_registry = parent_type::_texture_registry;
        
        
        pass.get_attachment_group().add_attachment( _swapchain->present_textures, glm::vec4(0.0f));
        
        _cube.create();
        subpass_type& sub_p = pass.add_subpass(parent_type::_material_store, "display_3d_texture");
        
        vk::resource_set<vk::texture_3d>& tex = _tex_registry->get_read_texture_3d_set(_texture, this,
                                                                vk::usage_type::COMBINED_IMAGE_SAMPLER);
        
        sub_p.set_image_sampler(tex, "texture_3d", vk::parameter_stage::FRAGMENT, 2,
                                       vk::usage_type::COMBINED_IMAGE_SAMPLER);
        
        int binding = 0;
        sub_p.init_parameter("mvp", vk::parameter_stage::VERTEX,
                             glm::mat4(1.0f), binding);
        sub_p.init_parameter("model", vk::parameter_stage::VERTEX,
                             glm::mat4(1.0f), binding);
        
        binding = 1;
        
        sub_p.init_parameter("box_eye_position", vk::parameter_stage::FRAGMENT,
                             glm::vec4(1.0f), binding);
        sub_p.init_parameter("screen_height", vk::parameter_stage::FRAGMENT,
                             (float)_swapchain->get_vk_swap_extent().height, binding);
        sub_p.init_parameter("screen_width", vk::parameter_stage::FRAGMENT,
                             (float)_swapchain->get_vk_swap_extent().width, binding);
        
        sub_p.add_output_attachment( _swapchain->present_textures.get_name().c_str());
        
        sub_p.set_cull_mode(vk::standard_pipeline::cull_mode::BACK_FACE);
        pass.add_object(_cube);
        
    }
    
    virtual void update_node(vk::camera& camera, uint32_t image_id) override
    {
        
        EA_ASSERT_MSG(_three_d_cam != nullptr, "No 3D camera has been specified to be used for rendering");
        render_pass_type &pass = parent_type::_node_render_pass;
        
        subpass_type& sub_p = pass.get_subpass(0);
        
        _three_d_cam->update_view_matrix();
        glm::mat4 mvp = _three_d_cam->get_projection_matrix() * _three_d_cam->view_matrix * glm::mat4(1.0f);
        
        vk::shader_parameter::shader_params_group& vertex_params =
            sub_p.get_pipeline(image_id).get_uniform_parameters(vk::parameter_stage::VERTEX, 0);
                
        vertex_params["mvp"] = mvp;
        vertex_params["model"] = glm::mat4(1.0f);
        
        vk::shader_parameter::shader_params_group& fragment_params = sub_p.get_pipeline(image_id).
                                                get_uniform_parameters(vk::parameter_stage::FRAGMENT, 1) ;
        
        fragment_params["box_eye_position"] =   glm::vec4(_three_d_cam->position, 1.0f);
        
    }
    
    virtual bool record_node_commands(vk::command_recorder& buffer, uint32_t image_id) override
    {
        parent_type::record_node_commands(buffer, image_id);
        return false;
    }
    
    
    virtual void destroy() override
    {
        _cube.destroy();
        parent_type::destroy();
    }
    
private:
    
    using parent_type::add_object;

    vk::obj_shape _cube;
    vk::glfw_swapchain* _swapchain = nullptr;
    vk::perspective_camera* _three_d_cam = nullptr;
    const char* _texture = nullptr;
};
