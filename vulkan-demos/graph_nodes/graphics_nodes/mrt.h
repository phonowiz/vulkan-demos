//
//  mrt.cpp
//  vulkan-demos
//
//  Created by Rafael Sabino on 4/17/20.
//  Copyright © 2020 Rafael Sabino. All rights reserved.
//

#pragma once

#include "graphics_node.h"
#include "material_store.h"
#include "texture_registry.h"
#include "voxelize.h"
#include "mip_map_3d_texture.hpp"


template<uint32_t NUM_CHILDREN>
class mrt: public vk::graphics_node<5, NUM_CHILDREN>
{
private:
    vk::orthographic_camera _ortho_camera;
    
public:
    
    enum class rendering_mode
    {
        ALBEDO = 0,
        NORMALS,
        POSITIONS,
        DEPTH,
        FULL_RENDERING,
        AMBIENT_OCCLUSION,
        AMBIENT_LIGHT,
        DIRECT_LIGHT
    };
    
    using parent_type = vk::graphics_node<5, NUM_CHILDREN>;
    using render_pass_type = typename parent_type::render_pass_type;
    using subpass_type = typename parent_type::render_pass_type::subpass_s;
    using object_vector_type = typename parent_type::object_vector_type;
    using image_ptr = std::shared_ptr<vk::image>;
    using tex_registry_type = typename parent_type::tex_registry_type;
    using material_store_type = typename parent_type::material_store_type;
    using object_submask_type = typename parent_type::object_subpass_mask;
    
    mrt(vk::device* dev, vk::glfw_swapchain* swapchain):
    parent_type(dev, swapchain->get_vk_swap_extent().width, swapchain->get_vk_swap_extent().height),
    _ortho_camera(_voxel_world_dimensions.x, _voxel_world_dimensions.y, _voxel_world_dimensions.z),
    _screen_plane(dev)
    {
        _swapchain= swapchain;
        _screen_plane.create();
    }
    
    
    void set_light_pos(glm::vec3 v)
    {
        _light_pos = v;
    }
    
    virtual void init_node() override
    {
        render_pass_type &pass = parent_type::_node_render_pass;
        //object_vector_type &obj_vec = parent_type::_obj_vector;
        tex_registry_type* _tex_registry = parent_type::_texture_registry;
        material_store_type* _mat_store = parent_type::_material_store;
        //object_submask_type& _obj_masks = parent_type::_obj_subpass_mask;
        object_vector_type& _obj_vector = parent_type::_obj_vector;
        
        subpass_type& mrt_subpass = pass.add_subpass(_mat_store, "mrt");
        subpass_type& composite = pass.add_subpass(_mat_store, "deferred_output");
        
        
        setup_sampling_rays();
        
        pass.add_object(_screen_plane);
        pass.skip_subpass(_screen_plane, 0);
        
        assert(_obj_vector.size() != 0);
        for(int i = 0; i < _obj_vector.size(); ++i)
        {
            pass.add_object(*_obj_vector[i]);
            pass.skip_subpass( *_obj_vector[i], 1);
        }
        
        vk::attachment_group<5>& mrt_attachment_group = pass.get_attachment_group();
        
        vk::resource_set<vk::render_texture>& normals = _tex_registry->get_write_render_texture_set("normals", this, vk::image::usage_type::INPUT_ATTACHMENT);
        vk::resource_set<vk::render_texture>& albedos = _tex_registry->get_write_render_texture_set("albedos", this, vk::image::usage_type::INPUT_ATTACHMENT);
        
        //TODO: you can derive positon from depth and sampling fragment position
        vk::resource_set<vk::render_texture>& positions = _tex_registry->get_write_render_texture_set("positions", this, vk::image::usage_type::INPUT_ATTACHMENT);
        vk::resource_set<vk::depth_texture>& depth = _tex_registry->get_write_depth_texture_set("depth", this, vk::image::usage_type::INPUT_ATTACHMENT);
        
        
        //TODO: MAKE MAKE SURE TO INITIALIZE ATTACHMENTS MANUALLY WHEN YOU REMOVE ATTACHMENT INIT FROM RENDERPASS INIT
        //TODO: add_attachment CALLS SHOULD BE ACCOMPANIED WITH A STRING NAME SO WE CAN REFERENCE IT.
        //GBUFFER SUBPASS
        //follow the order in which the attachments are expected in the shader
        mrt_attachment_group.add_attachment(normals, glm::vec4(0.0f));
        mrt_attachment_group.add_attachment(albedos, glm::vec4(0.0f));
        mrt_attachment_group.add_attachment(positions, glm::vec4(0.0f));
        mrt_attachment_group.add_attachment(_swapchain->present_textures, glm::vec4(0.0f));
        mrt_attachment_group.add_attachment(depth, 1.0f, 0.0f);
        
        //TODO: GET RID OF THIS AND REPLACE IT WITH STRING NAMES
        enum
        {
            NORMALS_ATTACHMENT_ID,
            ALBEDOS_ATTACHMENT_ID,
            POSITIONS_ATTACHMENT_ID,
            PRESENT_ATTACHMENT_ID,
            DEPTH_ATTACHMENT_ID
        };
        
        glm::vec2 dims = parent_type::_node_render_pass.get_dimensions();
        
        normals.set_format(vk::image::formats::R8G8_SIGNED_NORMALIZED);
        normals.set_dimensions(dims.x, dims.y);
        normals.set_filter(vk::image::filter::NEAREST);
        
        albedos.set_dimensions(dims.x, dims.y);
        albedos.set_filter(vk::image::filter::NEAREST);
        
        positions.set_dimensions(dims.x, dims.y);
        positions.set_filter(vk::image::filter::NEAREST);
        
        depth.set_format(vk::image::formats::DEPTH_32_FLOAT);
        depth.set_dimensions(dims.x, dims.y);
        depth.set_filter(vk::image::filter::NEAREST);
        
        normals.init();
        albedos.init();
        positions.init();
        depth.init();
        
        //TODO: set_number_of_blend_attachments function could now go away...
        mrt_subpass.set_number_of_blend_attachments(3);
        mrt_subpass.modify_attachment_blend(NORMALS_ATTACHMENT_ID, render_pass_type::write_channels::RG, false);
        mrt_subpass.modify_attachment_blend(ALBEDOS_ATTACHMENT_ID, render_pass_type::write_channels::RGBA, false);
        mrt_subpass.modify_attachment_blend(POSITIONS_ATTACHMENT_ID, render_pass_type::write_channels::RGBA, false);
        
        mrt_subpass.add_output_attachment("normals");
        mrt_subpass.add_output_attachment("albedos" );
        mrt_subpass.add_output_attachment("positions");
        mrt_subpass.add_output_attachment("depth");
        
        
        int binding = 0;
        //note: parameters are initialized in the order found in the shader
        mrt_subpass.init_parameter("view", vk::visual_material::parameter_stage::VERTEX, glm::mat4(0), binding);
        mrt_subpass.init_parameter("projection", vk::visual_material::parameter_stage::VERTEX, glm::mat4(0), binding);
        mrt_subpass.init_parameter("lightPosition", vk::visual_material::parameter_stage::VERTEX, glm::vec3(0), binding);
        
        parent_type::add_dynamic_param("model", 0, vk::visual_material::parameter_stage::VERTEX, glm::mat4(1.0), 1);
        
        
        //COMPOSITE SUBPASS
        composite.add_input_attachment( "normals", "normals", vk::visual_material::parameter_stage::FRAGMENT, 1 );
        composite.add_input_attachment("albedos", "albedos", vk::visual_material::parameter_stage::FRAGMENT, 2);
        composite.add_input_attachment("positions", "positions", vk::visual_material::parameter_stage::FRAGMENT, 3);

        composite.add_input_attachment("depth", "depth", vk::visual_material::parameter_stage::FRAGMENT, 4);
        
        composite.add_output_attachment("present");
        
        composite.init_parameter("width", vk::visual_material::parameter_stage::VERTEX, static_cast<float>(_swapchain->get_vk_swap_extent().width), 0);
        composite.init_parameter("height", vk::visual_material::parameter_stage::VERTEX, static_cast<float>(_swapchain->get_vk_swap_extent().height), 0);
        
        vk::resource_set<vk::texture_3d>& voxel_normal_set = _tex_registry->get_read_texture_3d_set("voxel_normals", this, vk::material_base::usage_type::COMBINED_IMAGE_SAMPLER);
        vk::resource_set<vk::texture_3d>& voxel_albedo_set = _tex_registry->get_read_texture_3d_set("voxel_albedos", this, vk::material_base::usage_type::COMBINED_IMAGE_SAMPLER);
        
        
        
        glm::vec4 world_scale_voxel = glm::vec4(float(_voxel_world_dimensions.x/voxelize<NUM_CHILDREN>::VOXEL_CUBE_WIDTH),
                                                float(_voxel_world_dimensions.y/voxelize<NUM_CHILDREN>::VOXEL_CUBE_HEIGHT),
                                                float(_voxel_world_dimensions.z/voxelize<NUM_CHILDREN>::VOXEL_CUBE_DEPTH), 1.0f);
        
        composite.init_parameter("world_cam_position", vk::visual_material::parameter_stage::FRAGMENT, glm::vec4(0.0f), 5);
        composite.init_parameter("world_light_position", vk::visual_material::parameter_stage::FRAGMENT, glm::vec3(0.0f), 5);
        composite.init_parameter("light_color", vk::visual_material::parameter_stage::FRAGMENT, glm::vec4(0.0f), 5);
        composite.init_parameter("voxel_size_in_world_space", vk::visual_material::parameter_stage::FRAGMENT, world_scale_voxel, 5);
        composite.init_parameter("mode", vk::visual_material::parameter_stage::FRAGMENT, int(0), 5);
        composite.init_parameter("sampling_rays", vk::visual_material::parameter_stage::FRAGMENT, _sampling_rays.data(), _sampling_rays.size(), 5);
        composite.init_parameter("vox_view_projection", vk::visual_material::parameter_stage::FRAGMENT, glm::mat4(1.0f), 5);
        composite.init_parameter("num_of_lods", vk::visual_material::parameter_stage::FRAGMENT, int(mip_map_3d_texture<NUM_CHILDREN>::TOTAL_LODS), 5);
        composite.init_parameter("eye_in_world_space", vk::visual_material::parameter_stage::FRAGMENT, glm::vec3(0), 5);
        composite.init_parameter("eye_inverse_view_matrix", vk::visual_material::parameter_stage::FRAGMENT, glm::mat4(1.0f), 5);
        
        composite.set_image_sampler(voxel_normal_set, "voxel_normals", vk::visual_material::parameter_stage::FRAGMENT, 6, vk::material_base::usage_type::COMBINED_IMAGE_SAMPLER);
        composite.set_image_sampler(voxel_albedo_set, "voxel_albedos", vk::visual_material::parameter_stage::FRAGMENT, 7, vk::material_base::usage_type::COMBINED_IMAGE_SAMPLER);
        
        static eastl::array<eastl::fixed_string<char, 100>, mip_map_3d_texture<NUM_CHILDREN>::TOTAL_LODS> albedo_lods;
        static eastl::array<eastl::fixed_string<char, 100>, mip_map_3d_texture<NUM_CHILDREN>::TOTAL_LODS> normal_lods;
        
        int binding_index = 8;
        int offset = 5;
        
        for( int i = 1; i < mip_map_3d_texture<NUM_CHILDREN>::TOTAL_LODS; ++i)
        {
            normal_lods[i].sprintf("voxel_normals%i", i);
            albedo_lods[i].sprintf("voxel_albedos%i", i);
            
            vk::resource_set<vk::texture_3d>& normal3d = _tex_registry->get_read_texture_3d_set(normal_lods[i].c_str(), this, vk::image::usage_type::COMBINED_IMAGE_SAMPLER);
            vk::resource_set<vk::texture_3d>& albedo3d = _tex_registry->get_read_texture_3d_set(albedo_lods[i].c_str(), this, vk::image::usage_type::COMBINED_IMAGE_SAMPLER);
            
            composite.set_image_sampler(albedo3d, albedo_lods[i].c_str(), vk::visual_material::parameter_stage::FRAGMENT, binding_index, vk::resource::usage_type::COMBINED_IMAGE_SAMPLER);
            composite.set_image_sampler(normal3d, normal_lods[i].c_str(), vk::visual_material::parameter_stage::FRAGMENT, binding_index + offset, vk::resource::usage_type::COMBINED_IMAGE_SAMPLER);
            binding_index++;
        }
        
    }
    
    virtual void update_node(vk::camera& camera, uint32_t image_id) override
    {
        render_pass_type &pass = parent_type::_node_render_pass;
        object_vector_type &obj_vec = parent_type::_obj_vector;
        //tex_registry_type* _tex_registry = parent_type::_texture_registry;
        //material_store_type* _mat_store = parent_type::_material_store;
        //object_submask_type& _obj_masks = parent_type::_obj_subpass_mask;
        //object_vector_type& _obj_vector = parent_type::_obj_vector;
        
        subpass_type& mrt_pass = pass.get_subpass(0);
        subpass_type& composite = pass.get_subpass(1);
        
        vk::shader_parameter::shader_params_group& vertex_params = mrt_pass.get_pipeline(image_id).
                                                get_uniform_parameters(vk::visual_material::parameter_stage::VERTEX, 0);
        
        vk::shader_parameter::shader_params_group& display_fragment_params = composite.get_pipeline(image_id).
                                                get_uniform_parameters(vk::visual_material::parameter_stage::FRAGMENT, 5) ;
        
        //TODO: THIS NEEDS TO MATCH THE VOXELIZER NODE DISTANCE...
        constexpr float distance = 8.f;
        _ortho_camera.position = { 0.0f, 0.0f, -distance};
        _ortho_camera.forward = -_ortho_camera.position;
        
        _ortho_camera.up = camera.up;
        _ortho_camera.update_view_matrix();
        
        vertex_params["view"] = camera.view_matrix;
        vertex_params["projection"] =  camera.get_projection_matrix();
        
        display_fragment_params["eye_inverse_view_matrix"] = glm::inverse(camera.view_matrix);
        display_fragment_params["vox_view_projection"] = _ortho_camera.get_projection_matrix() * _ortho_camera.view_matrix;
        display_fragment_params["eye_in_world_space"] = camera.position;

        display_fragment_params["world_cam_position"] = glm::vec4(camera.position, 1.0f);
        display_fragment_params["world_light_position"] = _light_pos;
        display_fragment_params["light_color"] = _light_color;
        display_fragment_params["mode"] = static_cast<int>(_rendering_mode);
        
        for( int i = 0; i < obj_vec.size(); ++i)
        {
            parent_type::set_dynamic_param("model", image_id, 0, obj_vec[i],
                                           obj_vec[i]->transform.get_transform_matrix(), 0 );
        }
        
    }
    
    inline void set_rendering_state( rendering_mode state ){ _rendering_mode = state; }
    
    virtual void destroy() override
    {
        parent_type::destroy();
        _screen_plane.destroy();
    }
    
private:
    
    rendering_mode _rendering_mode = rendering_mode::FULL_RENDERING;
    
    void setup_sampling_rays()
    {
        glm::vec4 up = glm::vec4(0.0f, 1.0f, .0f, 0.0f);
        _sampling_rays[0] = up;
        glm::vec4 temp = glm::vec4(1.0f, 1.0f, 0.f, 0.0f);
        _sampling_rays[1] = glm::normalize(temp);
        
        temp = glm::vec4(-1.0f, 1.0f, 0.f, 0.0f);
        _sampling_rays[2] = glm::normalize(temp);
        
        temp = glm::vec4(0.0f, 1.0f, 1.0f, 0.0f);
        _sampling_rays[3] = glm::normalize(temp);
        
        temp = glm::vec4(0.0f, 1.0f, -1.0f,0.0f);
        _sampling_rays[4] = glm::normalize(temp);
    }
    
    vk::screen_plane _screen_plane;
    vk::glfw_swapchain* _swapchain = nullptr;
    
    static constexpr glm::vec3 _voxel_world_dimensions = glm::vec3(10.0f, 10.0f, 10.0f);
    
    static constexpr size_t   NUM_SAMPLING_RAYS = 5;
    eastl::array<glm::vec4, NUM_SAMPLING_RAYS> _sampling_rays = {};
    
    glm::vec3 _light_pos = glm::vec3(0.0f, .8f, 0.0f);
    glm::vec4 _light_color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
};

template class mrt<4>;