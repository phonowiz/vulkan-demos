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
#include "glfw_swapchain.h"
#include "screen_plane.h"
#include "texture_2d.h"
#include "attachment_group.h"
#include "EASTL/fixed_string.h"
#include "EAStdC/EASprintf.h"


template< uint32_t NUM_CHILDREN>
class voxelize : public vk::graphics_node<1, NUM_CHILDREN>
{
    
private:
    static constexpr unsigned int TOTAL_LODS = 6;
    
    //eastl::array<eastl::fixed_string<char, 100>, TOTAL_LODS> str_array;
public:
    
    using parent_type = vk::graphics_node<1, NUM_CHILDREN>;
    using render_pass_type = typename parent_type::render_pass_type;
    using subpass_type = typename parent_type::render_pass_type::subpass_s;
    using object_vector_type = typename parent_type::object_vector_type;
    using image_ptr = std::shared_ptr<vk::image>;
    using tex_registry_type = typename parent_type::tex_registry_type;
    using material_store_type = typename parent_type::material_store_type;
    
    voxelize(vk::device* dev, vk::glfw_swapchain* swapchain, uint32_t width, uint32_t height):
    parent_type(dev, width, height)
    {
        
    }
    
    virtual void init_node() override
    {
        render_pass_type &pass = parent_type::_node_render_pass;
        object_vector_type &obj_vec = parent_type::_obj_vector;
        tex_registry_type* _tex_registry = parent_type::_texture_registry;
        material_store_type* _mat_store = parent_type::_material_store;
        
        subpass_type& voxelize_subpass = pass.add_subpass(_mat_store, "voxelizer");
        
        enum{ VOXEL_ATTACHMENT_ID = 0 };
        voxelize_subpass.add_output_attachment(VOXEL_ATTACHMENT_ID);
        
        voxelize_subpass.set_number_of_blend_attachments(1);
        voxelize_subpass.modify_attachment_blend(0, render_pass_type::write_channels::RGBA, false);
        
        //for( int i = 0; i < str_array.size(); ++i )
        {
            //str_array[i].sprintf("voxel_albedo_texture_%i", i);
            vk::resource_set<vk::texture_3d>& albedo_textures = _tex_registry->get_write_texture_3d_set("voxel_albedo_texture", this);
            vk::resource_set<vk::texture_3d>& normal_textures = _tex_registry->get_write_texture_3d_set("voxel_normal_texture", this);
            
            voxelize_subpass.set_image_sampler(albedo_textures, "voxel_albedo_texture",
                                               vk::visual_material::parameter_stage::FRAGMENT, 6, vk::material_base::usage_type::COMBINED_IMAGE_SAMPLER );
            
            voxelize_subpass.set_image_sampler(normal_textures, "voxel_normal_texture",
                                               vk::visual_material::parameter_stage::FRAGMENT, 7, vk::material_base::usage_type::COMBINED_IMAGE_SAMPLER );
        }
        
//        voxelize_subpass.set_image_sampler(_voxel_albedo_textures[0], "voxel_albedo_texture",
//                                             vk::visual_material::parameter_stage::FRAGMENT, 1, vk::resource::usage_type::STORAGE_IMAGE);
//        voxelize_subpass.set_image_sampler(_voxel_normal_textures[0], "voxel_normal_texture",
//                                             vk::visual_material::parameter_stage::FRAGMENT, 4, vk::resource::usage_type::STORAGE_IMAGE);
        
        voxelize_subpass.init_parameter("inverse_view_projection", vk::visual_material::parameter_stage::FRAGMENT, glm::mat4(1.0f), 2);
        voxelize_subpass.init_parameter("project_to_voxel_screen", vk::visual_material::parameter_stage::FRAGMENT, glm::mat4(1.0f), 2);
        voxelize_subpass.init_parameter("voxel_coords", vk::visual_material::parameter_stage::FRAGMENT, glm::vec3(1.0f), 2);
        
        voxelize_subpass.init_parameter("view", vk::visual_material::parameter_stage::VERTEX, glm::mat4(1.0f), 0);
        voxelize_subpass.init_parameter("projection", vk::visual_material::parameter_stage::VERTEX, glm::mat4(1.0f), 0);
        voxelize_subpass.init_parameter("light_position", vk::visual_material::parameter_stage::VERTEX, glm::vec3(1.0f), 0);
        voxelize_subpass.init_parameter("eye_position", vk::visual_material::parameter_stage::VERTEX, glm::vec3(1.0f), 0);
        
        //voxelize_subpass.init_dynamic_params("model", vk::visual_material::parameter_stage::VERTEX, glm::mat4(1.0), shapes.size(), 3);
        
        voxelize_subpass.set_cull_mode( render_pass_type::graphics_pipeline_type::cull_mode::NONE);
        
        
    }
    
    virtual void update(vk::camera& camera, uint32_t image_id) override
    {

    }
    
    virtual void destroy() override
    {
        parent_type::destroy();
    }
    
};


template class voxelize<1>;
