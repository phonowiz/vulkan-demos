//
//  vsm.cpp
//  vulkan-demos
//
//  Created by Rafael Sabino on 5/8/20.
//  Copyright © 2020 Rafael Sabino. All rights reserved.
//

#pragma once

#include "EAAssert/eaassert.h"
#include "graphics_node.h"



static constexpr uint32_t NUM_ATTACHMENTS = 2;
//variance shadow maps
template< uint32_t NUM_CHILDREN>
class vsm : public vk::graphics_node<NUM_ATTACHMENTS, NUM_CHILDREN>
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
    
    vk::camera* _light_cam = nullptr;
    
    vk::resource_set<vk::render_texture>* _vsm = nullptr;
    
public:
    
    using parent_type = vk::graphics_node<NUM_ATTACHMENTS, NUM_CHILDREN>;
    using render_pass_type = typename parent_type::render_pass_type;
    using subpass_type = typename parent_type::render_pass_type::subpass_s;
    using object_vector_type = typename parent_type::object_vector_type;
    using image_ptr = eastl::shared_ptr<vk::image>;
    using tex_registry_type = typename parent_type::tex_registry_type;
    using material_store_type = typename parent_type::material_store_type;
    using object_submask_type = typename parent_type::object_subpass_mask;
    
    
    vsm(vk::device* dev, float width, float height, vk::camera& cam):
    parent_type(dev, width, height)
    {
        _light_cam = &cam;
    }
    
    virtual void init_node() override
    {
        render_pass_type &pass = parent_type::_node_render_pass;
        object_vector_type &obj_vec = parent_type::_obj_vector;
        tex_registry_type* _tex_registry = parent_type::_texture_registry;
        material_store_type* _mat_store = parent_type::_material_store;
        object_vector_type& _obj_vector = parent_type::_obj_vector;
        
        EA_ASSERT_MSG(_light_cam != nullptr, "light cam has not been initialize");
        
        subpass_type& cam_depth_subpass = pass.add_subpass(_mat_store, "vsm");
         
        vk::resource_set<vk::render_texture>& vsm =  _tex_registry->get_write_render_texture_set("vsm", this);
        vk::resource_set<vk::depth_texture>& vsm_depth = _tex_registry->get_write_depth_texture_set("vsm_depth", this, vk::usage_type::INPUT_ATTACHMENT);
        
        
        for( int i = 0; i < vsm_depth.size(); ++i)
        {
            vsm_depth[i].set_write_to_texture(true);
        }
        _vsm = &vsm;
        vk::attachment_group<NUM_ATTACHMENTS>& vsm_attachment_grp = pass.get_attachment_group();
        
        vsm_attachment_grp.add_attachment(vsm, glm::vec4(1.0f));
        vsm_attachment_grp.add_attachment(vsm_depth, glm::vec2(1.0f, 0.0f));
        
        vsm.set_format(vk::image::formats::R8G8_SIGNED_NORMALIZED);
        
        glm::vec2 dims = parent_type::_node_render_pass.get_dimensions();
        vsm.set_dimensions(dims.x, dims.y);
        vsm.set_filter(vk::image::filter::NEAREST);
        vsm.set_format(vk::image::formats::R32G32_SIGNED_FLOAT);
        vsm.init();
        vsm_depth.init();
        
        cam_depth_subpass.add_output_attachment("vsm", render_pass_type::write_channels::RGBA, true);
        cam_depth_subpass.add_output_attachment("vsm_depth", render_pass_type::write_channels::R, true);
        
        cam_depth_subpass.set_cull_mode(vk::graphics_pipeline<NUM_ATTACHMENTS>::cull_mode::NONE );
        
        cam_depth_subpass.init_parameter("view", vk::parameter_stage::VERTEX, glm::mat4(1.0f), 0);
        cam_depth_subpass.init_parameter("projection", vk::parameter_stage::VERTEX, glm::mat4(1.0f), 0);
        
        for(int i = 0; i < _obj_vector.size(); ++i)
        {
            pass.add_object(_obj_vector[i]->get_lod(1));
        }
        
        parent_type::add_dynamic_param("model", 0, vk::parameter_stage::VERTEX, glm::mat4(1.0), 1);
        

    }
    
    virtual void update_node(vk::camera& camera, uint32_t image_id) override
    {

        render_pass_type &pass = parent_type::_node_render_pass;
        object_vector_type &obj_vec = parent_type::_obj_vector;
//        tex_registry_type* _tex_registry = parent_type::_texture_registry;
//        material_store_type* _mat_store = parent_type::_material_store;
//        object_submask_type& _obj_masks = parent_type::_obj_subpass_mask;
//        object_vector_type& _obj_vector = parent_type::_obj_vector;
        
        EA_ASSERT_MSG(_light_cam, "light cam has not been assigned");
        subpass_type& vsm_subpass = pass.get_subpass(0);
        vk::shader_parameter::shader_params_group& vsm_vertex_params =
                vsm_subpass.get_pipeline(image_id).get_uniform_parameters(vk::parameter_stage::VERTEX, 0);
        
        _light_cam->update_view_matrix();
        vsm_vertex_params["view"] = _light_cam->view_matrix;
        vsm_vertex_params["projection"] = _light_cam->get_projection_matrix();
        
        
        for( int i = 0; i < obj_vec.size(); ++i)
        {
            parent_type::set_dynamic_param("model", image_id, 0, obj_vec[i]->get_lod(1),
                                           obj_vec[i]->transforms[image_id].get_transform_matrix(), 1 );
        }
        
    }
    
    virtual void destroy() override
    {
        parent_type::destroy();
    }
};
