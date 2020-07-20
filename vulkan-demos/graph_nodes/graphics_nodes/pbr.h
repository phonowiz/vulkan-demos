//
//  diffuse.hpp
//  vulkan-demos
//
//  Created by Rafael Sabino on 6/19/20.
//  Copyright © 2020 Rafael Sabino. All rights reserved.
//

#pragma once

#include "graphics_node.h"

static const uint32_t ATTACHMENTS = 4;
template< uint32_t NUM_CHILDREN>
class pbr : public vk::graphics_node<ATTACHMENTS, NUM_CHILDREN>
{
public:
    
    using parent_type = vk::graphics_node<ATTACHMENTS, NUM_CHILDREN>;
    using render_pass_type = typename parent_type::render_pass_type;
    using subpass_type = typename parent_type::render_pass_type::subpass_s;
    using object_vector_type = typename parent_type::object_vector_type;
    using image_ptr = eastl::shared_ptr<vk::image>;
    using tex_registry_type = typename parent_type::tex_registry_type;
    using material_store_type = typename parent_type::material_store_type;
    using object_submask_type = typename parent_type::object_subpass_mask;
    
    material_store_type* _mat_store = parent_type::_material_store;
    object_vector_type& _obj_vector = parent_type::_obj_vector;
    
    
    pbr(vk::device* dev, uint32_t width, uint32_t height):
    parent_type(dev, width, height)
    { }
    
    //this node is heavy on multisampling, for an excellent explanation of how this works in vulkan, go here:
    //https://vulkan-tutorial.com/Multisampling
    virtual void init_node() override
    {
        render_pass_type &pass = parent_type::_node_render_pass;
        object_vector_type &obj_vec = parent_type::_obj_vector;
        tex_registry_type* _tex_registry = parent_type::_texture_registry;
        material_store_type* _mat_store = parent_type::_material_store;
        object_vector_type& _obj_vector = parent_type::_obj_vector;
        
        vk::resource_set<vk::render_texture>& albedos =  _tex_registry->get_write_render_texture_set("albedos",
                                                                                                 this, vk::usage_type::INPUT_ATTACHMENT);
        
        vk::resource_set<vk::render_texture>& normals =  _tex_registry->get_write_render_texture_set("normals",
                                                                                                 this, vk::usage_type::INPUT_ATTACHMENT);
        
        
        //TODO: you can derive positon from depth and sampling fragment position
        vk::resource_set<vk::render_texture>& positions = _tex_registry->get_write_render_texture_set("positions", this, vk::usage_type::INPUT_ATTACHMENT);
        vk::resource_set<vk::depth_texture>& depth = _tex_registry->get_write_depth_texture_set("depth", this, vk::usage_type::INPUT_ATTACHMENT);

        vk::attachment_group<ATTACHMENTS>& pbr_attachment_group = pass.get_attachment_group();
        
        //all color attachments must have a resolve attachment for the multisampling to work
        pbr_attachment_group.add_attachment(albedos, glm::vec4(0));
        pbr_attachment_group.add_attachment(normals, glm::vec4(0));
        pbr_attachment_group.add_attachment(positions, glm::vec4(0));
        pbr_attachment_group.add_attachment(depth, glm::vec2(1.0f, 0.0f), true, true);
        
        albedos.set_filter(vk::image::filter::NEAREST);

        depth.set_format(vk::image::formats::DEPTH_32_FLOAT);
        depth.set_filter(vk::image::filter::NEAREST);
        
        normals.set_format(vk::image::formats::R32G32B32A32_SIGNED_FLOAT);
        normals.set_filter(vk::image::filter::NEAREST);

        positions.set_filter(vk::image::filter::NEAREST);
        positions.set_format(vk::image::formats::R32G32B32A32_SIGNED_FLOAT);
        
        albedos.init();
        normals.init();
        positions.init();
        depth.init();
        
        for(int i = 0; i < _obj_vector.size(); ++i)
        {
            
            vk::texture_path diffuse_texture = _obj_vector[i]->get_lod(0)->get_texture((uint32_t)(aiTextureType_BASE_COLOR));
            vk::texture_path specular_texture = _obj_vector[i]->get_lod(0)->get_texture((uint32_t)(aiTextureType_METALNESS));
            vk::texture_path normals_texture = _obj_vector[i]->get_lod(0)->get_texture((uint32_t)(aiTextureType_NORMAL_CAMERA));
            vk::texture_path roughness_texture = _obj_vector[i]->get_lod(0)->get_texture((uint32_t)(aiTextureType_DIFFUSE_ROUGHNESS));
            vk::texture_path ao_texture = _obj_vector[i]->get_lod(0)->get_texture((uint32_t)(aiTextureType_AMBIENT_OCCLUSION));
            

            subpass_type& pbr =  pass.add_subpass(_mat_store,"pbr");
            pbr.add_output_attachment("albedos", render_pass_type::write_channels::RGBA, false);
            pbr.add_output_attachment("normals", render_pass_type::write_channels::RGBA, false);
            pbr.add_output_attachment("positions", render_pass_type::write_channels::RGBA, false);
            pbr.add_output_attachment("depth");
                
            
            vk::texture_2d& diffuse = _tex_registry->get_loaded_texture(diffuse_texture.c_str(), this, parent_type::_device, diffuse_texture.c_str());
            vk::texture_2d& norms = _tex_registry->get_loaded_texture(normals_texture.c_str(), this, parent_type::_device, normals_texture.c_str());
            vk::texture_2d& metals = _tex_registry->get_loaded_texture(specular_texture.c_str(), this, parent_type::_device, specular_texture.c_str());
            vk::texture_2d& roughness = _tex_registry->get_loaded_texture(roughness_texture.c_str(), this, parent_type::_device, roughness_texture.c_str());
            vk::texture_2d& occlusion = _tex_registry->get_loaded_texture(ao_texture.c_str(), this, parent_type::_device, ao_texture.c_str());
            pass.add_object(_obj_vector[i]->get_lod(0));
            
            roughness.set_filter(vk::image::filter::LINEAR);
            roughness.init();
            
            norms.init();
            metals.init();
            roughness.init();
            occlusion.init();
            
            pbr.init_parameter("view", vk::parameter_stage::VERTEX, glm::mat4(0), 0);
            pbr.init_parameter("projection", vk::parameter_stage::VERTEX, glm::mat4(0), 0);
            
            pbr.set_image_sampler( diffuse, "albedos",
                                  vk::parameter_stage::FRAGMENT, 2, vk::usage_type::COMBINED_IMAGE_SAMPLER);
            pbr.set_image_sampler( norms, "normals",
                                  vk::parameter_stage::FRAGMENT, 3, vk::usage_type::COMBINED_IMAGE_SAMPLER);
            pbr.set_image_sampler( metals, "metalness",
                                  vk::parameter_stage::FRAGMENT, 4, vk::usage_type::COMBINED_IMAGE_SAMPLER);
            pbr.set_image_sampler( roughness, "roughness",
                                  vk::parameter_stage::FRAGMENT, 5, vk::usage_type::COMBINED_IMAGE_SAMPLER);
            pbr.set_image_sampler( occlusion, "occlusion",
                                  vk::parameter_stage::FRAGMENT, 6, vk::usage_type::COMBINED_IMAGE_SAMPLER);
            
            pbr.ignore_all_objs(true);
            pbr.ignore_object(i, false);
            
            parent_type::add_dynamic_param("model", i, vk::parameter_stage::VERTEX, glm::mat4(1.0), 1);
        }
    }
    
    virtual void update_node(vk::camera& camera, uint32_t image_id) override
    {
        render_pass_type &pass = parent_type::_node_render_pass;
        object_vector_type &obj_vec = parent_type::_obj_vector;
        
        for(int i = 0; i < _obj_vector.size(); ++i)
        {
            subpass_type& pbr_subpass = pass.get_subpass(i);
            vk::shader_parameter::shader_params_group& pbr_vertex_params =
                    pbr_subpass.get_pipeline(image_id).get_uniform_parameters(vk::parameter_stage::VERTEX, 0);
            
            pbr_vertex_params["view"] = camera.view_matrix;
            pbr_vertex_params["projection"] = camera.get_projection_matrix();
            
            parent_type::set_dynamic_param("model", image_id, i, obj_vec[i]->get_lod(0),
                                           obj_vec[i]->transforms[image_id].get_transform_matrix(), 1 );
        }
    }
    
    virtual void destroy() override
    {
        parent_type::destroy();
    }
    
private:
    
};

pbr<1>;
