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
    
    //if changing these values, make sure the voxelize.vert shader
    //reflects your change as well.
    enum class light_type
    {
        DIRECTIONAL_LIGHT = 0,
        POINT_LIGHT = 1
    };
    
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
    
    vk::camera _key_light_cam;
    
    glm::mat4 _proj_to_voxel_screen = glm::mat4(1.0f);
    
    glm::vec3 _light_pos = glm::vec3(0.0f, .8f, 0.0f);
    light_type _light_type = light_type::DIRECTIONAL_LIGHT;
    
public:
    
    using parent_type = vk::graphics_node<1, NUM_CHILDREN>;
    using render_pass_type = typename parent_type::render_pass_type;
    using subpass_type = typename parent_type::render_pass_type::subpass_s;
    using object_vector_type = typename parent_type::object_vector_type;
    using image_ptr = eastl::shared_ptr<vk::image>;
    using tex_registry_type = typename parent_type::tex_registry_type;
    using material_store_type = typename parent_type::material_store_type;
    using object_submask_type = typename parent_type::object_subpass_mask;
    
    voxelize( ):
    _ortho_camera(WORLD_VOXEL_SIZE, WORLD_VOXEL_SIZE, WORLD_VOXEL_SIZE)
    {}
    
    voxelize(vk::device* dev, vk::camera& key_light_cam):
    parent_type(dev, static_cast<float>(VOXEL_CUBE_WIDTH), static_cast<float>(VOXEL_CUBE_HEIGHT)),
    _ortho_camera(WORLD_VOXEL_SIZE, WORLD_VOXEL_SIZE, WORLD_VOXEL_SIZE)
    {
        _key_light_cam = key_light_cam;
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
  
    void set_key_light_cam(vk::camera& key_light_cam, light_type type)
    {
        _light_type = type;
        _key_light_cam = key_light_cam;
    }
    
    
private:
    void set_vertex_args(subpass_type& type, int use_texture)
    {
        type.init_parameter("view", vk::parameter_stage::VERTEX, glm::mat4(1.0f), 0);
        type.init_parameter("projection", vk::parameter_stage::VERTEX, glm::mat4(1.0f), 0);
        type.init_parameter("light_position", vk::parameter_stage::VERTEX, glm::vec3(1.0f), 0);
        type.init_parameter("eye_position", vk::parameter_stage::VERTEX, glm::vec3(1.0f), 0);
        type.init_parameter("light_type", vk::parameter_stage::VERTEX, int(_light_type), 0);
        type.init_parameter("use_texture", vk::parameter_stage::VERTEX, use_texture, 0);
        
    }
    
public:
    virtual void init_node() override
    {
        render_pass_type &pass = parent_type::_node_render_pass;
        object_vector_type &obj_vec = parent_type::_obj_vector;
        tex_registry_type* _tex_registry = parent_type::_texture_registry;
        material_store_type* _mat_store = parent_type::_material_store;
        object_vector_type& _obj_vector = parent_type::_obj_vector;
        
        for(int i = 0; i < _obj_vector.size(); ++i)
        {
            pass.add_object( _obj_vector[i]->get_lod(0) );
        }
        
        vk::texture_2d& black = _tex_registry->get_loaded_texture("black.png", this, parent_type::_device,"black.png");
        black.init();
        vk::attachment_group<1>& attachment_group = pass.get_attachment_group();
        
        eastl::fixed_string<char, 100> test_name {};
        
        //TODO: MAKE IT SO THAT WE CAN RE-USE THE SAME TEXTURE BETWEEN THE VOXELIZATION  NODES
        test_name.sprintf("vox_test<%f, %f, %f>", _cam_position.x, _cam_position.y, _cam_position.z );
        vk::resource_set<vk::render_texture>& target = _tex_registry->get_write_render_texture_set(test_name.c_str(),
                                                                                                   this);
        
        target.set_device(parent_type::_device);
        target.set_dimensions(float(VOXEL_CUBE_WIDTH), float(VOXEL_CUBE_HEIGHT));
        
        target.init();
        
        attachment_group.add_attachment(target, glm::vec4(1.0f, 1.0f, 1.0f, .0f));
        enum{ VOXEL_ATTACHMENT_ID = 0 };
        
        for( int obj = 0; obj < _obj_vector.size(); ++obj )
        {
            int use_texture = 1;
            
            subpass_type& voxelize_subpass = pass.add_subpass(_mat_store, "voxelizer");
            vk::texture_path diffuse = _obj_vector[obj]->get_lod(0)->get_texture((uint32_t)(aiTextureType_BASE_COLOR));
            
            if(!diffuse.empty())
            {
                vk::texture_2d& rsrc = _tex_registry->get_loaded_texture(diffuse.c_str(), this, parent_type::_device, diffuse.c_str());
                rsrc.init();
                voxelize_subpass.set_image_sampler( rsrc, "albedos",
                                      vk::parameter_stage::VERTEX, 5);
            }
            else
            {
                use_texture = 0;
                voxelize_subpass.set_image_sampler( black, "albedos",
                                      vk::parameter_stage::VERTEX, 5);
            }
            
            set_vertex_args(voxelize_subpass, use_texture);
            
            voxelize_subpass.ignore_all_objs(true);
            voxelize_subpass.ignore_object(obj, false);
            
            vk::resource_set<vk::texture_3d>& albedo_textures = _tex_registry->get_write_texture_3d_set("voxel_albedos", this);
            vk::resource_set<vk::texture_3d>& normal_textures = _tex_registry->get_write_texture_3d_set("voxel_normals", this);
            
            voxelize_subpass.set_image_sampler(albedo_textures, "voxel_albedo_texture", vk::parameter_stage::FRAGMENT, 1 );
            voxelize_subpass.set_image_sampler(normal_textures, "voxel_normal_texture", vk::parameter_stage::FRAGMENT, 4 );
            
            voxelize_subpass.init_parameter("inverse_view_projection", vk::parameter_stage::FRAGMENT, glm::mat4(1.0f), 2);
            voxelize_subpass.init_parameter("project_to_voxel_screen", vk::parameter_stage::FRAGMENT, _proj_to_voxel_screen, 2);
            voxelize_subpass.init_parameter("voxel_coords", vk::parameter_stage::FRAGMENT,
                                                glm::vec3(VOXEL_CUBE_WIDTH,VOXEL_CUBE_HEIGHT, VOXEL_CUBE_DEPTH ), 2);
            
            parent_type::add_dynamic_param("model", obj, vk::parameter_stage::VERTEX, glm::mat4(1.0), 3);
            voxelize_subpass.set_cull_mode( render_pass_type::graphics_pipeline_type::cull_mode::NONE);
            
            voxelize_subpass.add_output_attachment(test_name.c_str(), render_pass_type::write_channels::RGBA, false);
        }

    }
    
    virtual void update_node(vk::camera& camera, uint32_t image_id) override
    {

        render_pass_type &pass = parent_type::_node_render_pass;
        object_vector_type &obj_vec = parent_type::_obj_vector;
        tex_registry_type* _tex_registry = parent_type::_texture_registry;
        material_store_type* _mat_store = parent_type::_material_store;
        object_vector_type& _obj_vector = parent_type::_obj_vector;
        
        for( int i = 0; i < _obj_vector.size(); ++i)
        {
            subpass_type& vox_subpass = pass.get_subpass(i);
            vk::shader_parameter::shader_params_group& voxelize_vertex_params =
                    vox_subpass.get_pipeline(image_id).get_uniform_parameters(vk::parameter_stage::VERTEX, 0);
            
            vk::shader_parameter::shader_params_group& voxelize_frag_params =
                    vox_subpass.get_pipeline(image_id).get_uniform_parameters(vk::parameter_stage::FRAGMENT, 2);
            
            
            _ortho_camera.position = _cam_position;
            _ortho_camera.forward = -_ortho_camera.position;
            
            _ortho_camera.up = _up_vector;
            _ortho_camera.update_view_matrix();
            
            glm::mat4 ivp = _ortho_camera.get_projection_matrix() * _ortho_camera.view_matrix;
            ivp = glm::inverse( ivp );
            voxelize_frag_params["inverse_view_projection"] = ivp;
            
            voxelize_vertex_params["view"] = _ortho_camera.view_matrix;
            voxelize_vertex_params["projection"] =_ortho_camera.get_projection_matrix();
            voxelize_vertex_params["light_position"] = _key_light_cam.position;
            voxelize_vertex_params["eye_position"] = camera.position;
    

            parent_type::set_dynamic_param("model", image_id, i, _obj_vector[i]->get_lod(0),
                                           _obj_vector[i]->transforms[image_id].get_transform_matrix(), 3 );
        }
    }
    
    virtual void destroy() override
    {
        parent_type::destroy();
    }
    
};


template class voxelize<1>;
