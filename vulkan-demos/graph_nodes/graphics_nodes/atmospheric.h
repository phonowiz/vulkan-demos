//
//  atmospheric.h
//  vulkan-demos
//
//  Created by Rafael Sabino on 7/31/20.
//  Copyright © 2020 Rafael Sabino. All rights reserved.
//

#pragma once

#include "graphics_node.h"
#include "texture_cube.h"


static const uint32_t ATMOSPHERIC_ATTACHMENTS = 6;

template< uint32_t NUM_CHILDREN>
class atmospheric : public vk::graphics_node<ATMOSPHERIC_ATTACHMENTS, NUM_CHILDREN>
{
public:
    static constexpr  uint32_t ENVIRONMENT_DIMENSIONS = 128;
private:
    vk::screen_plane _screen_plane;
    

    float _planet_radius = 6371e3f;
    glm::vec4 _sun_position = glm::vec4(0.0f, _planet_radius, 0.0f, 0.0f);
    
    eastl::array<const char*, 5> _directions =
    {
        "positive_x",
        "negative_x",
        "positive_y",
        "positive_z",
        "negative_z"
    };
    
public:
    
    using parent_type = vk::graphics_node<ATMOSPHERIC_ATTACHMENTS, NUM_CHILDREN>;
    using render_pass_type = typename parent_type::render_pass_type;
    using subpass_type = typename parent_type::render_pass_type::subpass_s;
    using object_vector_type = typename parent_type::object_vector_type;
    using image_ptr = eastl::shared_ptr<vk::image>;
    using tex_registry_type = typename parent_type::tex_registry_type;
    using material_store_type = typename parent_type::material_store_type;
    using object_submask_type = typename parent_type::object_subpass_mask;
    

    void set_sun_position(glm::vec3 position)
    {
        position = glm::normalize(position) * _planet_radius;
        _sun_position.x = position.x;
        _sun_position.y = position.y;
        _sun_position.z = position.z;
    }
    
    atmospheric(vk::device* dev):
    parent_type(dev,ENVIRONMENT_DIMENSIONS ,ENVIRONMENT_DIMENSIONS),
    _screen_plane(dev)
    {
    }
    
    virtual void init_node() override
    {
        render_pass_type &pass = parent_type::_node_render_pass;
        object_vector_type &obj_vec = parent_type::_obj_vector;
        tex_registry_type* _tex_registry = parent_type::_texture_registry;
        material_store_type* _mat_store = parent_type::_material_store;
        object_vector_type& _obj_vector = parent_type::_obj_vector;
       
        _screen_plane.create();
        
        vk::resource_set<vk::texture_cube>& atmospheric = _tex_registry->get_write_texture_cube_set("atmospheric", this, vk::usage_type::INPUT_ATTACHMENT);

        atmospheric.set_filter(vk::image::filter::LINEAR);
        atmospheric.set_format(vk::image::formats::R32G32B32A32_SIGNED_FLOAT);
        atmospheric.set_dimensions(ENVIRONMENT_DIMENSIONS, ENVIRONMENT_DIMENSIONS);
        atmospheric.init();
        
        vk::attachment_group<ATMOSPHERIC_ATTACHMENTS>& atmospheric_grp = pass.get_attachment_group();
       
        atmospheric_grp.add_attachment( atmospheric, glm::vec4(0), true, true);
        for( int i = 0; i < 6; ++i)
        {
            subpass_type& atmospheric_subpass = pass.add_subpass(_mat_store, "atmospheric");
            atmospheric_subpass.set_image_sampler(atmospheric, "cubemap_texture", vk::parameter_stage::FRAGMENT, 0);

            atmospheric_subpass.init_parameter("positive_x", vk::parameter_stage::FRAGMENT, glm::mat4(1.0f), 1);
            
            atmospheric_subpass.init_parameter("ray_beta", vk::parameter_stage::FRAGMENT, glm::vec4(5.5e-6f, 13.0e-6f, 22.4e-6f, 0.0f), 1);
            atmospheric_subpass.init_parameter("mie_beta", vk::parameter_stage::FRAGMENT, glm::vec4(21e-6f), 1);
            atmospheric_subpass.init_parameter("ambient_beta", vk::parameter_stage::FRAGMENT, glm::vec3(0.0f), 1);
            atmospheric_subpass.init_parameter("absorption_beta", vk::parameter_stage::FRAGMENT, glm::vec4(2.04e-5f, 4.97e-5f, 1.95e-6f, 0.0f), 1);
            
            glm::vec4 planet_pos = glm::vec4(0.0f, -_planet_radius, 0.0f, 0.0f);
            atmospheric_subpass.init_parameter("planet_position", vk::parameter_stage::FRAGMENT, planet_pos, 1);
            
            atmospheric_subpass.init_parameter("light_direction", vk::parameter_stage::FRAGMENT, glm::vec4(0.0f), 1);

            atmospheric_subpass.init_parameter("cam_position", vk::parameter_stage::FRAGMENT, glm::vec4(0.0f), 1);
            atmospheric_subpass.init_parameter("screen_size", vk::parameter_stage::FRAGMENT, glm::vec2(ENVIRONMENT_DIMENSIONS, ENVIRONMENT_DIMENSIONS), 1);
            atmospheric_subpass.init_parameter("planet_radius", vk::parameter_stage::FRAGMENT, _planet_radius, 1);
            atmospheric_subpass.init_parameter("g", vk::parameter_stage::FRAGMENT, .7f, 1);
            atmospheric_subpass.init_parameter("view_steps", vk::parameter_stage::FRAGMENT, 64.0f, 1);
            atmospheric_subpass.init_parameter("light_steps", vk::parameter_stage::FRAGMENT, 4.f, 1);
        
            atmospheric_subpass.set_cull_mode(vk::graphics_pipeline<ATMOSPHERIC_ATTACHMENTS>::cull_mode::NONE);
            eastl::fixed_string<char, 100> name {};
            name.sprintf("%s_%i", "atmospheric", i);
            atmospheric_subpass.add_output_attachment(name.c_str());
        }
        pass.add_object(static_cast<vk::obj_shape*>(&_screen_plane));

    }
    
    virtual void update_node(vk::camera& camera, uint32_t image_id) override
    {
        render_pass_type &pass = parent_type::_node_render_pass;
        
        glm::vec3 direction_vectors[6] = {
            glm::vec3 {1.0f, 0.0f, 0.0f}, //+X
            glm::vec3 {-1.0f, 0.0f, 0.0f}, //-X
            glm::vec3 {0.0f, 1.0f, 0.0f}, //+Y
            glm::vec3 {0.0f, -1.0f, 0.0f}, //-Y
            glm::vec3 {0.0f, 0.0f, 1.0f}, //+Z
            glm::vec3 {0.0f, 0.0f, -1.0f} //-Z
        };
        glm::vec3 up[6] = {
            glm::vec3 {0.0f, 1.0f, 0.0f},
            glm::vec3 {0.0f, 1.0f, 0.0f},
            glm::vec3 {0.0f, 0.0f, -1.0f},
            glm::vec3 {0.0f, 0.0f, -1.0f},
            glm::vec3 {0.0f, 1.0f, 0.0f},
            glm::vec3 {0.0f, 1.0f, 0.0f}
        };
        
        constexpr float aspect = ENVIRONMENT_DIMENSIONS/ENVIRONMENT_DIMENSIONS;
        
        vk::perspective_camera atmos_cam(glm::radians(45.0f),
                                                  aspect, .01f, 100.0f);
        
        for(int i = 0; i < 6; ++i)
        {
            subpass_type& atmos_subpass = pass.get_subpass(i);
            vk::shader_parameter::shader_params_group& atmos_params = atmos_subpass.get_pipeline(image_id).
                                                    get_uniform_parameters(vk::parameter_stage::FRAGMENT, 1) ;
            
            atmos_cam.forward = direction_vectors[i];
            atmos_cam.up = up[i];
            atmos_cam.position = camera.position;
            atmos_cam.update_view_matrix();
            atmos_params["positive_x"] =  glm::transpose(atmos_cam.view_matrix);
            
            atmos_params["cam_position"] = glm::vec4(camera.position.x, camera.position.y, camera.position.z, 1.0f);
            glm::vec4 dir( _sun_position.x - camera.position.x, _sun_position.y - camera.position.y, _sun_position.z - camera.position.z, 0.0f);
            atmos_params["light_direction"] = glm::normalize(dir);
        }
    }
    
    virtual void destroy() override
    {
        _screen_plane.destroy();
        parent_type::destroy();
    }
    
};

template class atmospheric<1>;


