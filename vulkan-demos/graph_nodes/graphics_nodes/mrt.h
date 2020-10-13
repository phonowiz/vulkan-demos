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


static constexpr uint32_t MRT_ATTACHMENTS = 5;
template<uint32_t NUM_CHILDREN>
class mrt: public vk::graphics_node<MRT_ATTACHMENTS, NUM_CHILDREN>
{
    
public:
    
    using light_type = typename voxelize<NUM_CHILDREN>::light_type;

private:
    vk::orthographic_camera _ortho_camera;
    vk::camera _light_cam;
    
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
        DIRECT_LIGHT,
        VARIANCE_SHADOW_MAP
    };
    
    using parent_type = vk::graphics_node<MRT_ATTACHMENTS, NUM_CHILDREN>;
    using render_pass_type = typename parent_type::render_pass_type;
    using subpass_type = typename parent_type::render_pass_type::subpass_s;
    using object_vector_type = typename parent_type::object_vector_type;
    using image_ptr = eastl::shared_ptr<vk::image>;
    using tex_registry_type = typename parent_type::tex_registry_type;
    using material_store_type = typename parent_type::material_store_type;
    using object_submask_type = typename parent_type::object_subpass_mask;
    
    mrt(vk::device* dev, vk::glfw_swapchain* swapchain, vk::camera& key_light_cam, light_type light_type):
    parent_type(dev, swapchain->get_vk_swap_extent().width, swapchain->get_vk_swap_extent().height),
    _ortho_camera(_voxel_world_dimensions.x, _voxel_world_dimensions.y, _voxel_world_dimensions.z),
    _screen_plane(dev)
    {
        _swapchain= swapchain;
        _screen_plane.create();
        
        _light_cam = key_light_cam;
        _light_color[0] = glm::vec4(1.0f, 1.0f, 1.f, 1.0f);
        
        for( int i = 0; i < _light_types.size(); ++i)
        {
            _light_types[i] = static_cast<int>(light_type::DIRECTIONAL_LIGHT);
        }
    }

    void set_key_light_color( glm::vec3 color, uint32_t  i = 0 )
    {
        _light_color[i].x = color.x;
        _light_color[i].y = color.y;
        _light_color[i].z = color.z;
        _light_color[i].w = 1.0f;
    }
    virtual void init_node() override
    {
        render_pass_type &pass = parent_type::_node_render_pass; 
        tex_registry_type* _tex_registry = parent_type::_texture_registry;
        material_store_type* _mat_store = parent_type::_material_store;
        object_vector_type& _obj_vector = parent_type::_obj_vector;
        
        subpass_type& composite = pass.add_subpass(_mat_store, "deferred_output");
        
        setup_sampling_rays();
        
        pass.add_object(static_cast<vk::obj_shape*>(&_screen_plane));

        vk::attachment_group<MRT_ATTACHMENTS>& mrt_attachment_group = pass.get_attachment_group();
        
        vk::resource_set<vk::render_texture>& normals = _tex_registry->get_read_render_texture_set("normals", this, vk::usage_type::INPUT_ATTACHMENT);
        vk::resource_set<vk::render_texture>& albedos = _tex_registry->get_read_render_texture_set("albedos", this, vk::usage_type::INPUT_ATTACHMENT);
        
        //TODO: you can derive positon from depth and sampling fragment position
        vk::resource_set<vk::render_texture>& positions = _tex_registry->get_read_render_texture_set("positions", this, vk::usage_type::INPUT_ATTACHMENT);
        vk::resource_set<vk::depth_texture>& depth = _tex_registry->get_read_depth_texture_set("depth", this, vk::usage_type::INPUT_ATTACHMENT);
        
        
        vk::resource_set<vk::render_texture>& final_render =  _tex_registry->get_write_render_texture_set("final_render",this);
        
        vk::texture_cube& environment = _tex_registry->get_loaded_texture_cube("atmospheric", this, parent_type::_device, nullptr);
        vk::resource_set<vk::texture_cube>& spec_cubemap_high  = _tex_registry->get_read_texture_cube_set("spec_cubemap_high", this);
        vk::resource_set<vk::texture_cube>& spec_cubemap_low = _tex_registry->get_read_texture_cube_set("spec_cubemap_low", this);
        vk::resource_set<vk::texture_cube>& radiance_map = _tex_registry->get_read_texture_cube_set("radiance_map", this);
        vk::resource_set<vk::render_texture>& brdf_lut = _tex_registry->get_read_render_texture_set("spec_map_lut", this, vk::usage_type::COMBINED_IMAGE_SAMPLER);
        
        final_render.set_filter(vk::image::filter::LINEAR);
        final_render.set_format(vk::image::formats::R16G16B16A16_SIGNED_FLOAT);
        
        //GBUFFER SUBPASS
        mrt_attachment_group.add_attachment(normals, glm::vec4(0.0f), false, false);
        mrt_attachment_group.add_attachment(albedos, glm::vec4(0.0f), false, false);
        mrt_attachment_group.add_attachment(positions, glm::vec4(0.0f), false, false);
        mrt_attachment_group.add_attachment(final_render, glm::vec4(0.0f), true, true);
        mrt_attachment_group.add_attachment(depth, glm::vec4(0.0f), false, false);
        
        final_render.init();

        //COMPOSITE SUBPASS
        composite.add_input_attachment("normals", "normals", vk::parameter_stage::FRAGMENT, 1 );
        composite.add_input_attachment("albedos", "albedos", vk::parameter_stage::FRAGMENT, 2);
        composite.add_input_attachment("positions", "positions", vk::parameter_stage::FRAGMENT, 3);

        composite.add_input_attachment("depth", "depth", vk::parameter_stage::FRAGMENT, 4);
        
        composite.add_output_attachment("final_render", render_pass_type::write_channels::RGBA, false);
        
        composite.init_parameter("width", vk::parameter_stage::VERTEX, static_cast<float>(_swapchain->get_vk_swap_extent().width), 0);
        composite.init_parameter("height", vk::parameter_stage::VERTEX, static_cast<float>(_swapchain->get_vk_swap_extent().height), 0);
        
        vk::resource_set<vk::texture_3d>& voxel_normal_set = _tex_registry->get_read_texture_3d_set("voxel_normals", this);
        vk::resource_set<vk::texture_3d>& voxel_albedo_set = _tex_registry->get_read_texture_3d_set("voxel_albedos", this);
        
        
        vk::resource_set<vk::render_texture>& vsm_set = _tex_registry->get_read_render_texture_set("blur_final", this, vk::usage_type::COMBINED_IMAGE_SAMPLER);
        
        
        glm::vec4 world_scale_voxel = glm::vec4(float(_voxel_world_dimensions.x/voxelize<NUM_CHILDREN>::VOXEL_CUBE_WIDTH),
                                                float(_voxel_world_dimensions.y/voxelize<NUM_CHILDREN>::VOXEL_CUBE_HEIGHT),
                                                float(_voxel_world_dimensions.z/voxelize<NUM_CHILDREN>::VOXEL_CUBE_DEPTH), 1.0f);
        
        composite.init_parameter("world_cam_position", vk::parameter_stage::FRAGMENT, glm::vec4(0.0f), 5);
        composite.init_parameter("world_light_position", vk::parameter_stage::FRAGMENT, _world_light_positions.data(), _world_light_positions.size(), 5);
        composite.template init_parameter<MAX_LIGHTS>("light_color", vk::parameter_stage::FRAGMENT, _light_color, 5);
        composite.init_parameter("voxel_size_in_world_space", vk::parameter_stage::FRAGMENT, world_scale_voxel, 5);
        composite.init_parameter("mode", vk::parameter_stage::FRAGMENT, int(0), 5);
        composite.init_parameter("sampling_rays", vk::parameter_stage::FRAGMENT, _sampling_rays.data(), _sampling_rays.size(), 5);
        composite.init_parameter("vox_view_projection", vk::parameter_stage::FRAGMENT, glm::mat4(1.0f), 5);
        composite.init_parameter("num_of_lods", vk::parameter_stage::FRAGMENT, int(mip_map_3d_texture<NUM_CHILDREN>::TOTAL_LODS), 5);
        composite.init_parameter("eye_in_world_space", vk::parameter_stage::FRAGMENT, glm::vec3(0), 5);
        composite.init_parameter("eye_inverse_view_matrix", vk::parameter_stage::FRAGMENT, glm::mat4(1.0f), 5);
        composite.init_parameter("light_cam_proj_matrix", vk::parameter_stage::FRAGMENT, _light_cam.get_projection_matrix() * _light_cam.view_matrix, 5);
        composite.template init_parameter<MAX_LIGHTS>("light_types", vk::parameter_stage::FRAGMENT, _light_types, 5);
        composite.init_parameter("light_count", vk::parameter_stage::FRAGMENT, ACTIVE_LIGHTS, 5);
        composite.init_parameter("inverse_view_proj", vk::parameter_stage::FRAGMENT, glm::mat4(1.0f), 5);
        composite.init_parameter("screen_size", vk::parameter_stage::FRAGMENT,
                                 glm::vec2(_swapchain->get_vk_swap_extent().width, _swapchain->get_vk_swap_extent().height), 5);
        
        static eastl::array<eastl::fixed_string<char, 100>, mip_map_3d_texture<NUM_CHILDREN>::TOTAL_LODS> albedo_lods;
        static eastl::array<eastl::fixed_string<char, 100>, mip_map_3d_texture<NUM_CHILDREN>::TOTAL_LODS> normal_lods;
        
        int binding_index = 6;
        int offset = 4;
        
        for( int i = 2; i < mip_map_3d_texture<NUM_CHILDREN>::TOTAL_LODS; ++i)
        {
            normal_lods[i].sprintf("voxel_normals%i", i);
            albedo_lods[i].sprintf("voxel_albedos%i", i);
            
            vk::resource_set<vk::texture_3d>& normal3d = _tex_registry->get_read_texture_3d_set(normal_lods[i].c_str(), this);
            vk::resource_set<vk::texture_3d>& albedo3d = _tex_registry->get_read_texture_3d_set(albedo_lods[i].c_str(), this);
            
            composite.set_image_sampler(albedo3d, albedo_lods[i].c_str(), vk::parameter_stage::FRAGMENT, binding_index);
            composite.set_image_sampler(normal3d, normal_lods[i].c_str(), vk::parameter_stage::FRAGMENT, binding_index + offset);
            binding_index++;
        }
        
        eastl::array<char*, 3> ibl_samplers = { "radiance_map", "spec_cubemap_high", "spec_cubemap_low"};
        eastl::array<vk::resource_set<vk::texture_cube>*, 3> ibl_textures = { &radiance_map, &spec_cubemap_high, &spec_cubemap_low};
        
        composite.set_image_sampler(vsm_set, "vsm", vk::parameter_stage::FRAGMENT, binding_index + offset++);
        composite.set_image_sampler(environment, "environment", vk::parameter_stage::FRAGMENT, binding_index + offset++);
        
        int i = 0;
        for( ; i < ibl_samplers.size(); ++i)
        {
            composite.set_image_sampler(*ibl_textures[i], ibl_samplers[i], vk::parameter_stage::FRAGMENT, binding_index + offset++);
        }
        
        composite.set_image_sampler(brdf_lut, "brdfLUT", vk::parameter_stage::FRAGMENT, binding_index + offset++);
    }
    
    virtual void update_node(vk::camera& camera, uint32_t image_id) override
    {
        render_pass_type &pass = parent_type::_node_render_pass;
        object_vector_type &obj_vec = parent_type::_obj_vector;

        subpass_type& composite = pass.get_subpass(0);
        
        vk::shader_parameter::shader_params_group& display_fragment_params = composite.get_pipeline(image_id).
                                                get_uniform_parameters(vk::parameter_stage::FRAGMENT, 5) ;
        
        //TODO: THIS NEEDS TO MATCH THE VOXELIZER NODE DISTANCE...
        constexpr float distance = 8.f;
        _ortho_camera.position = { 0.0f, 0.0f, -distance};
        _ortho_camera.forward = -_ortho_camera.position;
        
        _ortho_camera.up = camera.up;
        _ortho_camera.update_view_matrix();
        
        display_fragment_params["eye_inverse_view_matrix"] = glm::transpose(camera.view_matrix);
        display_fragment_params["vox_view_projection"] = _ortho_camera.get_projection_matrix() * _ortho_camera.view_matrix;
        display_fragment_params["eye_in_world_space"] = camera.position;

        display_fragment_params["world_cam_position"] = glm::vec4(camera.position, 1.0f);
        display_fragment_params["inverse_view_proj"] = glm::transpose(camera.view_matrix) * glm::inverse(camera.get_projection_matrix());
        
        //the first light is the key light, and is also the only contributor to ambient light and shadows
        _world_light_positions[0].x = _light_cam.position.x;
        _world_light_positions[0].y = _light_cam.position.y;
        _world_light_positions[0].z = _light_cam.position.z;
        _world_light_positions[0].w = 1.0f;
    
        display_fragment_params["world_light_position"].set_vectors_array(_world_light_positions.data(),
                                                                            _world_light_positions.size());
        

        display_fragment_params["light_color"].set_vectors_array(_light_color.data(), _light_color.size());
        display_fragment_params["light_cam_proj_matrix"] = _light_cam.get_projection_matrix() * _light_cam.view_matrix;
        display_fragment_params["mode"] = static_cast<int>(_rendering_mode);
        
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
    
    //search for MAX_LIGHTS in shaders, if this variable changes here, you'll have to change it shaders too
    static constexpr int32_t   MAX_LIGHTS = 1;
    static constexpr int32_t   ACTIVE_LIGHTS = 1;
    
    eastl::array<glm::vec4, NUM_SAMPLING_RAYS>  _sampling_rays = {};
    eastl::array<glm::vec4, MAX_LIGHTS>         _world_light_positions = {};
    eastl::array<int, MAX_LIGHTS>               _light_types = {};
    
    //sunlight color by default
    eastl::array<glm::vec4 , MAX_LIGHTS> _light_color = {};
};

template class mrt<4>;
