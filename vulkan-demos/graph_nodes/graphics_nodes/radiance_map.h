//
//  radiance.h
//  vulkan-demos
//
//  Created by Rafael Sabino on 8/8/20.
//  Copyright © 2020 Rafael Sabino. All rights reserved.
//

#pragma once

#include "graphics_node.h"
#include "orthographic_camera.h"

static const uint32_t RADIANCE_ATTACHMENTS = 1;
template< uint32_t NUM_CHILDREN>
class radiance_map : public vk::graphics_node<RADIANCE_ATTACHMENTS, NUM_CHILDREN>
{
private:

    eastl::fixed_string<char,200> _cube_texture {};
    vk::screen_plane _screen_plane;
    
    eastl::array<const char*, 5> _directions = {
        "positive_x",
        "negative_x",
        "positive_y",
        //"negative_y", //we are assuming the bottom contributes nothing, change this if this isn't true for you
                        //if you change this here, you'll also have to change the radiance_map shader to accept new argument
        "positive_z",
        "negative_z"
    };
    
    glm::vec3 _direction_vectors[5] = {
        glm::vec3 {1.0f, 0.0f, 0.0f}, //+X
        glm::vec3 {-1.0f, 0.0f, 0.0f}, //-X
        glm::vec3 {0.0f, 1.0f, 0.0f}, //+Y
        glm::vec3 {0.0f, 0.0f, 1.0f}, //+Z
        glm::vec3 {0.0f, 0.0f, -1.0f} //-Z
    };
    glm::vec3 _up[5] = {
        glm::vec3 {0.0f, 1.0f, 0.0f},
        glm::vec3 {0.0f, 1.0f, 0.0f},
        glm::vec3 {0.0f, 0.0f, -1.0f},
        glm::vec3 {0.0f, 1.0f, 0.0f},
        glm::vec3 {0.0f, 1.0f, 0.0f}
    };
    
public:
    
    using parent_type = vk::graphics_node<RADIANCE_ATTACHMENTS, NUM_CHILDREN>;
    using render_pass_type = typename parent_type::render_pass_type;
    using subpass_type = typename parent_type::render_pass_type::subpass_s;
    using object_vector_type = typename parent_type::object_vector_type;
    using image_ptr = eastl::shared_ptr<vk::image>;
    using tex_registry_type = typename parent_type::tex_registry_type;
    using material_store_type = typename parent_type::material_store_type;
    using object_submask_type = typename parent_type::object_subpass_mask;
    
    radiance_map(vk::device* dev, const char* cube_texture, float width, float height):
    parent_type(dev,width ,height),
    _cube_texture(cube_texture),
    _screen_plane(dev)
    {}

    virtual void init_node() override
    {
        
        render_pass_type &pass = parent_type::_node_render_pass;
        object_vector_type &obj_vec = parent_type::_obj_vector;
        tex_registry_type* _tex_registry = parent_type::_texture_registry;
        material_store_type* _mat_store = parent_type::_material_store;
        object_vector_type& _obj_vector = parent_type::_obj_vector;

        _screen_plane.create();
        subpass_type& map = pass.add_subpass(_mat_store, "radiance_map");
        EA_ASSERT_MSG(!_cube_texture.empty(), "cube texture cannot be empty");
        vk::resource_set<vk::texture_cube>& cube_tex =  _tex_registry->get_read_texture_cube_set(_cube_texture.c_str(), this);
        vk::resource_set<vk::texture_cube>& radiance_tex =  _tex_registry->get_write_texture_cube_set("radiance_map", this);
        
        //TODO: in my version of vulkan, we need atttachments otherwise validation layers will throw errors,
        //TODO: in future versions of vulkan there is a device feature which allows imageless frame buffers.
        vk::resource_set<vk::render_texture>& dummy = _tex_registry->get_write_render_texture_set("dummy", this);

        vk::attachment_group<FXAA_ATTACHMENTS>& map_attachments = pass.get_attachment_group();
        map_attachments.add_attachment( dummy, glm::vec4(0.0f), true, true );
        
        dummy.init();

        radiance_tex.set_filter(vk::image::filter::LINEAR);
        radiance_tex.set_format(vk::image::formats::R32G32B32A32_SIGNED_FLOAT);
        glm::vec3 dims = cube_tex.get_dimensions();
        radiance_tex.set_dimensions(dims.x, dims.y);
        radiance_tex.init();

        map.set_image_sampler(cube_tex, "cubemap", vk::parameter_stage::FRAGMENT, 0, vk::usage_type::COMBINED_IMAGE_SAMPLER);
        map.set_image_sampler(radiance_tex, "radiance_map", vk::parameter_stage::FRAGMENT, 1, vk::usage_type::STORAGE_IMAGE);

        
        //float delta_phi = (2.0f * float(M_PI)) / 180.0f;
        float delta_phi = (2.0f * float(M_PI)) / 3.f;
        float delta_theta = (0.5f * float(M_PI)) / 5.0f;
        
        EA_ASSERT_MSG(_directions.size() == 5, "you've added one more direction that is not handled here, you'll need to also change the shader"
                      "to accept new parameter");

        
        map.init_parameter("delta_phi", vk::parameter_stage::FRAGMENT, delta_phi, 2);
        map.init_parameter("delta_theta", vk::parameter_stage::FRAGMENT, delta_theta, 2);
        map.init_parameter("screen_size", vk::parameter_stage::FRAGMENT, glm::vec2(dims.x, dims.y), 2);
        //for cubemaps, width and height are always equal.
        constexpr float aspect = 1.0f;
        
        vk::perspective_camera radiance_cam(glm::radians(45.0f),
                                                  1, .01f, 100.0f);
        for(int i = 0; i < _directions.size(); ++i)
        {
            radiance_cam.forward = _direction_vectors[i];
            radiance_cam.up = _up[i];
            radiance_cam.position = glm::vec3(.0f);
            radiance_cam.update_view_matrix();
            
            map.init_parameter(_directions[i], vk::parameter_stage::FRAGMENT, glm::transpose(radiance_cam.view_matrix),  2);
        }
        
        map.set_cull_mode(vk::standard_pipeline::cull_mode::NONE);
        map.add_output_attachment("dummy");
        pass.add_object(static_cast<vk::obj_shape*>(&_screen_plane));
    }
    
    virtual void update_node(vk::camera& camera, uint32_t image_id) override
    {
    
    }
    
    virtual void destroy() override
    {
        _screen_plane.destroy();
        parent_type::destroy();
    }
};

template class radiance_map<1>;
