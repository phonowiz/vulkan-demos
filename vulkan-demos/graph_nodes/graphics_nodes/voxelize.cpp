//
//  voxelize.cpp
//  vulkan-demos
//
//  Created by Rafael Sabino on 4/16/20.
//  Copyright © 2020 Rafael Sabino. All rights reserved.
//

#pragma once


#include "graphics_node.h"
#include "material_store.h"
#include "texture_registry.h"
//#include "glfw_swapchain.h"
#include "screen_plane.h"
#include "texture_2d.h"
#include "attachment_group.h"
#include "EASTL/fixed_string.h"
#include "EAStdC/EASprintf.h"
#include "orthographic_camera.h"



template< uint32_t NUM_CHILDREN>
class voxelize : public vk::graphics_node<1, NUM_CHILDREN>
{
    
public:
    static constexpr uint32_t VOXEL_CUBE_WIDTH = 256u;
    static constexpr uint32_t VOXEL_CUBE_HEIGHT = 256u;
    static constexpr uint32_t VOXEL_CUBE_DEPTH  =  256u ;
    static constexpr unsigned int TOTAL_LODS = 6;
    
private:
    
    static constexpr float WORLD_VOXEL_SIZE = 10.0f;
    static constexpr glm::vec3 _voxel_world_dimensions = glm::vec3(WORLD_VOXEL_SIZE, WORLD_VOXEL_SIZE, WORLD_VOXEL_SIZE);
    
    glm::mat4 proj_to_voxel_screen = glm::mat4(1.0f);
    
    vk::orthographic_camera _ortho_camera;
    glm::vec3 _cam_position {};
    glm::vec3 _up_vector{};
    
    glm::mat4 _proj_to_voxel_screen = glm::mat4(1.0f);
    
    glm::vec3 _light_pos = glm::vec3(0.0f, .8f, 0.0f);
    
public:
    
    using parent_type = vk::graphics_node<1, NUM_CHILDREN>;
    using render_pass_type = typename parent_type::render_pass_type;
    using subpass_type = typename parent_type::render_pass_type::subpass_s;
    using object_vector_type = typename parent_type::object_vector_type;
    using image_ptr = std::shared_ptr<vk::image>;
    using tex_registry_type = typename parent_type::tex_registry_type;
    using material_store_type = typename parent_type::material_store_type;
    using object_submask_type = typename parent_type::object_subpass_mask;
    
    voxelize(){}
    voxelize(vk::device* dev):
    parent_type(dev, static_cast<float>(VOXEL_CUBE_WIDTH), static_cast<float>(VOXEL_CUBE_HEIGHT)),
    _ortho_camera(WORLD_VOXEL_SIZE, WORLD_VOXEL_SIZE, WORLD_VOXEL_SIZE)
    {
        
    }
    
    void set_cam_params(glm::vec3 cam_pos, glm::vec3 up)
    {
        _cam_position = cam_pos;
        _up_vector = up;
    }
    
    void set_proj_to_voxel_screen(glm::mat4 mat)
    {
        _proj_to_voxel_screen = mat;
    }
    
    void set_light_pos(glm::vec3 v)
    {
        _light_pos = v;
    }
    
    virtual void init_node() override
    {
        render_pass_type &pass = parent_type::_node_render_pass;
        object_vector_type &obj_vec = parent_type::_obj_vector;
        tex_registry_type* _tex_registry = parent_type::_texture_registry;
        material_store_type* _mat_store = parent_type::_material_store;
        object_submask_type& _obj_masks = parent_type::_obj_subpass_mask;
        object_vector_type& _obj_vector = parent_type::_obj_vector;
        
        parent_type::debug_print("initializing node voxelize!!!");
        subpass_type& voxelize_subpass = pass.add_subpass(_mat_store, "voxelizer");
        
        
        voxelize_subpass.set_number_of_blend_attachments(1);
        voxelize_subpass.modify_attachment_blend(0, render_pass_type::write_channels::RGBA, false);
        
        vk::resource_set<vk::texture_3d>& albedo_textures = _tex_registry->get_write_texture_3d_set("voxel_albedos", this);
        vk::resource_set<vk::texture_3d>& normal_textures = _tex_registry->get_write_texture_3d_set("voxel_normals", this);
        
        voxelize_subpass.set_image_sampler(albedo_textures, "voxel_albedo_texture",
                                           vk::visual_material::parameter_stage::FRAGMENT, 6, vk::material_base::usage_type::COMBINED_IMAGE_SAMPLER );
        
        for( int i = 0; i < normal_textures.size(); ++i)
        {
            normal_textures[i].set_device(parent_type::_device);
            albedo_textures[i].set_device(parent_type::_device);
            
            normal_textures[i].set_dimensions(VOXEL_CUBE_WIDTH, VOXEL_CUBE_HEIGHT, VOXEL_CUBE_DEPTH);
            albedo_textures[i].set_dimensions(VOXEL_CUBE_WIDTH, VOXEL_CUBE_HEIGHT, VOXEL_CUBE_DEPTH);
            
            normal_textures[i].init();
            albedo_textures[i].init();
        }
        
        voxelize_subpass.set_image_sampler(normal_textures, "voxel_normal_texture",
                                           vk::visual_material::parameter_stage::FRAGMENT, 7, vk::material_base::usage_type::COMBINED_IMAGE_SAMPLER );
        
        voxelize_subpass.init_parameter("inverse_view_projection", vk::visual_material::parameter_stage::FRAGMENT, glm::mat4(1.0f), 2);
        voxelize_subpass.init_parameter("project_to_voxel_screen", vk::visual_material::parameter_stage::FRAGMENT, _proj_to_voxel_screen, 2);
        voxelize_subpass.init_parameter("voxel_coords", vk::visual_material::parameter_stage::FRAGMENT,
                                            glm::vec3(VOXEL_CUBE_WIDTH,VOXEL_CUBE_HEIGHT, VOXEL_CUBE_DEPTH ), 2);
        
        voxelize_subpass.init_parameter("view", vk::visual_material::parameter_stage::VERTEX, glm::mat4(1.0f), 0);
        voxelize_subpass.init_parameter("projection", vk::visual_material::parameter_stage::VERTEX, glm::mat4(1.0f), 0);
        voxelize_subpass.init_parameter("light_position", vk::visual_material::parameter_stage::VERTEX, glm::vec3(1.0f), 0);
        voxelize_subpass.init_parameter("eye_position", vk::visual_material::parameter_stage::VERTEX, glm::vec3(1.0f), 0);
        
        
        parent_type::add_dynamic_param("model", 0, vk::visual_material::parameter_stage::VERTEX, glm::mat4(1.0), 3);
        
        voxelize_subpass.set_cull_mode( render_pass_type::graphics_pipeline_type::cull_mode::NONE);
        
        vk::attachment_group<1>& attachment_group = pass.get_attachment_group();
        
        eastl::fixed_string<char, 100> test_name;
        test_name.sprintf("vox_test<%f, %f, %f>", _cam_position.x, _cam_position.y, _cam_position.z );
        vk::resource_set<vk::render_texture>& target = _tex_registry->get_write_render_texture_set(test_name.c_str(),
                                                                                                   this, vk::image::usage_type::INPUT_ATTACHMENT);
        
        for( int i = 0; i < target.size(); ++i)
        {
            target[i].set_device(parent_type::_device);
            target[i].set_dimensions(float(VOXEL_CUBE_WIDTH), float(VOXEL_CUBE_HEIGHT), float(VOXEL_CUBE_DEPTH));
            target[i].init();
        }
        
        attachment_group.add_attachment(target);
        
        enum{ VOXEL_ATTACHMENT_ID = 0 };
        voxelize_subpass.add_output_attachment(VOXEL_ATTACHMENT_ID);
        
        
    }
    
    virtual void update(vk::camera& camera, uint32_t image_id) override
    {

        render_pass_type &pass = parent_type::_node_render_pass;
        object_vector_type &obj_vec = parent_type::_obj_vector;
        tex_registry_type* _tex_registry = parent_type::_texture_registry;
        material_store_type* _mat_store = parent_type::_material_store;
        object_submask_type& _obj_masks = parent_type::_obj_subpass_mask;
        object_vector_type& _obj_vector = parent_type::_obj_vector;
        
        
        subpass_type& vox_subpass = pass.get_subpass(0);
        vk::shader_parameter::shader_params_group& voxelize_vertex_params =
                vox_subpass.get_pipeline(image_id).get_uniform_parameters(vk::visual_material::parameter_stage::VERTEX, 0);
        
        vk::shader_parameter::shader_params_group& voxelize_frag_params =
                vox_subpass.get_pipeline(image_id).get_uniform_parameters(vk::visual_material::parameter_stage::FRAGMENT, 2);
        
        
        _ortho_camera.position = _cam_position;
        _ortho_camera.forward = -_ortho_camera.position;
        
        _ortho_camera.up = _up_vector;
        _ortho_camera.update_view_matrix();
        
        voxelize_frag_params["inverse_view_projection"] = glm::inverse( _ortho_camera.get_projection_matrix() * _ortho_camera.view_matrix);
        voxelize_vertex_params["view"] = _ortho_camera.view_matrix;
        voxelize_vertex_params["projection"] =_ortho_camera.get_projection_matrix();

        voxelize_vertex_params["light_position"] = _light_pos;
        voxelize_vertex_params["eye_position"] = camera.position;
    
        for( int i = 0; i < _obj_vector.size(); ++i)
        {
            parent_type::set_dynamic_param("model", image_id, 0, _obj_vector[i],
                                           _obj_vector[i]->transform.get_transform_matrix(), 3 );
        }
        
        vox_subpass.commit_parameters_to_gpu(image_id);
    }
    
    virtual void destroy() override
    {
        parent_type::destroy();
    }
    
};


template class voxelize<1>;
