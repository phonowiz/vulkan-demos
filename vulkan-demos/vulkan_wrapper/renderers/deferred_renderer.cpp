//
//  deferred_renderer.cpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 4/17/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#include "deferred_renderer.h"
#include "glfw_swapchain.h"
#include "texture_2d.h"
#include "../shapes/meshes/mesh.h"

#include <iostream>
#include <assert.h>

using namespace vk;

deferred_renderer::deferred_renderer(device* device, GLFWwindow* window, vk::glfw_swapchain* swapchain, vk::material_store& store, std::vector<obj_shape*>& shapes):
renderer(device, window, swapchain, store.GET_MAT<visual_material>("deferred_output")),
_screen_plane(device),
_mrt_pipeline(device,  glm::vec2(swapchain->get_vk_swap_extent().width, swapchain->get_vk_swap_extent().height), store.GET_MAT<visual_material>("mrt")),
_voxelize_pipeline(device, glm::vec2(VOXEL_CUBE_WIDTH, VOXEL_CUBE_HEIGHT),  store.GET_MAT<visual_material>("voxelizer")),
_ortho_camera(_voxel_world_dimensions.x, _voxel_world_dimensions.y, _voxel_world_dimensions.z)
{
    int binding = 0;
    
    for( int chain_id = 0; chain_id < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++chain_id)
    {
        for( int texture_id = 0; texture_id < _g_buffer_textures.size(); ++texture_id)
        {
            _g_buffer_textures[texture_id][chain_id].set_device(device);
            _g_buffer_textures[texture_id][chain_id].set_dimensions(swapchain->get_vk_swap_extent().width, swapchain->get_vk_swap_extent().height, 1);
            _g_buffer_textures[texture_id][chain_id].set_filter(image::filter::NEAREST);
            
            if( buffer_ids::NORMALS == texture_id) _g_buffer_textures[texture_id][chain_id].set_format(image::formats::R8G8_SIGNED_NORMALIZED);
            
            _g_buffer_textures[texture_id][chain_id].init();
        }
        
        _voxel_2d_view[0][chain_id].set_device(device);
        _voxel_2d_view[0][chain_id].set_dimensions(VOXEL_CUBE_WIDTH, VOXEL_CUBE_HEIGHT, 1);
        _voxel_2d_view[0][chain_id].set_image_layout(image::image_layouts::SHADER_READ_ONLY_OPTIMAL);
        _voxel_2d_view[0][chain_id].set_filter(image::filter::NEAREST);
        _voxel_2d_view[0][chain_id].init();
    }

    
    for( int lod_id = 0; lod_id < TOTAL_LODS; ++lod_id)
    {
        for( size_t j = 0; j < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++j)
        {
            _clear_voxel_texture_pipeline[lod_id][j].set_device(_device);
            _clear_voxel_texture_pipeline[lod_id][j].set_material(store.GET_MAT<compute_material>("clear_3d_texture"));
            
            _voxel_albedo_textures[lod_id][j].set_device(device);
            _voxel_albedo_textures[lod_id][j].set_dimensions(VOXEL_CUBE_WIDTH >> lod_id,  VOXEL_CUBE_HEIGHT >> lod_id, VOXEL_CUBE_DEPTH >> lod_id );
            
            _voxel_normal_textures[lod_id][j].set_device(device);
            _voxel_normal_textures[lod_id][j].set_dimensions(VOXEL_CUBE_WIDTH >> lod_id,  VOXEL_CUBE_HEIGHT >> lod_id, VOXEL_CUBE_DEPTH >> lod_id );
            
            _voxel_albedo_textures[lod_id][j].set_filter(image::filter::LINEAR);
            _voxel_normal_textures[lod_id][j].set_filter(image::filter::LINEAR);
            
            _voxel_normal_textures[lod_id][j].set_image_layout(image::image_layouts::SHADER_READ_ONLY_OPTIMAL);
            _voxel_albedo_textures[lod_id][j].set_image_layout(image::image_layouts::SHADER_READ_ONLY_OPTIMAL);
            
            assert(_voxel_albedo_textures[lod_id][j].get_height() % compute_pipeline::LOCAL_GROUP_SIZE == 0 && "voxel texture will not clear properly with these dimensions");
            assert(_voxel_albedo_textures[lod_id][j].get_width() % compute_pipeline::LOCAL_GROUP_SIZE == 0 && "voxel texture will not clear properly with these dimensions");
            assert(_voxel_albedo_textures[lod_id][j].get_depth() % compute_pipeline::LOCAL_GROUP_SIZE == 0 && "voxel texture will not clear properly with these dimensions");
        }
    }
    
    for( int lod_ids = 0; lod_ids < TOTAL_LODS; ++lod_ids)
    {
        for( size_t i = 0; i < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++i)
        {
            _voxel_albedo_textures[lod_ids][i].init();
            _voxel_normal_textures[lod_ids][i].init();
        }
    }
    
    for( int i = 0; i < shapes.size(); ++i)
    {
        add_shape(shapes[i]);
    }
    
    setup_sampling_rays();

    //MRT pipeline
    _mrt_pipeline.set_rendering_attachments(_g_buffer_textures);
    _mrt_pipeline.set_depth_enable(true);
    _mrt_pipeline.set_offscreen_rendering(true);
    
    _mrt_pipeline.init_parameter("view", visual_material::parameter_stage::VERTEX, glm::mat4(0), binding);
    _mrt_pipeline.init_parameter("projection", visual_material::parameter_stage::VERTEX, glm::mat4(0), binding);
    _mrt_pipeline.init_parameter("lightPosition", visual_material::parameter_stage::VERTEX, glm::vec3(0), binding);
    
    _mrt_pipeline.set_number_of_blend_attachments(3);
    _mrt_pipeline.modify_attachment_blend(0, mrt_pipeline::write_channels::RGBA, false);
    _mrt_pipeline.modify_attachment_blend(1, mrt_pipeline::write_channels::RGBA, false);
    _mrt_pipeline.modify_attachment_blend(2, mrt_pipeline::write_channels::RGBA, false);
    glm::mat4 identity = glm::mat4(1);
    _mrt_pipeline.init_dynamic_params("model", visual_material::parameter_stage::VERTEX, identity, shapes.size(), 1);

    _mrt_pipeline.create();
    
    //voxelize pipeline
    _voxelize_pipeline.set_number_of_blend_attachments(1);
    _voxelize_pipeline.modify_attachment_blend(0, voxelize_pipeline::write_channels::RGBA, false);
    
    _voxelize_pipeline.set_image_sampler(_voxel_albedo_textures[0], "voxel_albedo_texture",
                                         visual_material::parameter_stage::FRAGMENT, 1, resource::usage_type::STORAGE_IMAGE);
    _voxelize_pipeline.set_image_sampler(_voxel_normal_textures[0], "voxel_normal_texture",
                                         visual_material::parameter_stage::FRAGMENT, 4, resource::usage_type::STORAGE_IMAGE);
    
    _voxelize_pipeline.init_parameter("inverse_view_projection", visual_material::parameter_stage::FRAGMENT, glm::mat4(1.0f), 2);
    _voxelize_pipeline.init_parameter("project_to_voxel_screen", visual_material::parameter_stage::FRAGMENT, glm::mat4(1.0f), 2);
    _voxelize_pipeline.init_parameter("voxel_coords", visual_material::parameter_stage::FRAGMENT, glm::vec3(1.0f), 2);
    
    _voxelize_pipeline.init_parameter("view", visual_material::parameter_stage::VERTEX, glm::mat4(1.0f), 0);
    _voxelize_pipeline.init_parameter("projection", visual_material::parameter_stage::VERTEX, glm::mat4(1.0f), 0);
    _voxelize_pipeline.init_parameter("light_position", visual_material::parameter_stage::VERTEX, glm::vec3(1.0f), 0);
    _voxelize_pipeline.init_parameter("eye_position", visual_material::parameter_stage::VERTEX, glm::vec3(1.0f), 0);
    _voxelize_pipeline.init_dynamic_params("model", visual_material::parameter_stage::VERTEX, identity, shapes.size(), 3);
    
    _voxelize_pipeline.set_cullmode( voxelize_pipeline::cull_mode::NONE);
    _voxelize_pipeline.set_depth_enable(false);
    _voxelize_pipeline.set_rendering_attachments(_voxel_2d_view );
    _voxelize_pipeline.create();
    

    //pipeline
    _pipeline.init_parameter("width", visual_material::parameter_stage::VERTEX, 0.f, 0);
    _pipeline.init_parameter("height", visual_material::parameter_stage::VERTEX, 0.f, 0);
    
    _pipeline.set_image_sampler(_g_buffer_textures[buffer_ids::NORMALS], "normals", visual_material::parameter_stage::FRAGMENT, 1, resource::usage_type::COMBINED_IMAGE_SAMPLER);
    _pipeline.set_image_sampler(_g_buffer_textures[buffer_ids::ALBEDOS], "albedo", visual_material::parameter_stage::FRAGMENT, 2, resource::usage_type::COMBINED_IMAGE_SAMPLER);
    _pipeline.set_image_sampler(_g_buffer_textures[buffer_ids::POSITIONS], "world_positions", visual_material::parameter_stage::FRAGMENT, 3, resource::usage_type::COMBINED_IMAGE_SAMPLER);
    _pipeline.set_image_sampler(_mrt_pipeline.get_depth_textures(), "depth", visual_material::parameter_stage::FRAGMENT, 4, resource::usage_type::COMBINED_IMAGE_SAMPLER);
    _pipeline.set_image_sampler(_voxel_normal_textures[0], "voxel_normals", visual_material::parameter_stage::FRAGMENT, 6, material_base::usage_type::COMBINED_IMAGE_SAMPLER);
    _pipeline.set_image_sampler(_voxel_albedo_textures[0], "voxel_albedos", visual_material::parameter_stage::FRAGMENT, 7, material_base::usage_type::COMBINED_IMAGE_SAMPLER);
    
    glm::vec4 world_scale_voxel = glm::vec4(float(_voxel_world_dimensions.x/VOXEL_CUBE_WIDTH),
                                            float(_voxel_world_dimensions.y/VOXEL_CUBE_HEIGHT),
                                            float(_voxel_world_dimensions.z/VOXEL_CUBE_DEPTH), 1.0f);
    
    _pipeline.init_parameter("world_cam_position", visual_material::parameter_stage::FRAGMENT, glm::vec4(0.0f), 5);
    _pipeline.init_parameter("world_light_position", visual_material::parameter_stage::FRAGMENT, glm::vec3(0.0f), 5);
    _pipeline.init_parameter("light_color", visual_material::parameter_stage::FRAGMENT, glm::vec4(0.0f), 5);
    _pipeline.init_parameter("voxel_size_in_world_space", visual_material::parameter_stage::FRAGMENT, world_scale_voxel, 5);
    _pipeline.init_parameter("mode", visual_material::parameter_stage::FRAGMENT, int(0), 5);
    _pipeline.init_parameter("sampling_rays", visual_material::parameter_stage::FRAGMENT, _sampling_rays.data(), _sampling_rays.size(), 5);
    _pipeline.init_parameter("vox_view_projection", visual_material::parameter_stage::FRAGMENT, glm::mat4(1.0f), 5);
    _pipeline.init_parameter("num_of_lods", visual_material::parameter_stage::FRAGMENT, int(TOTAL_LODS), 5);
    _pipeline.init_parameter("eye_in_world_space", visual_material::parameter_stage::FRAGMENT, glm::vec3(0), 5);
    _pipeline.init_parameter("eye_inverse_view_matrix", visual_material::parameter_stage::FRAGMENT, glm::mat4(1.0f), 5);
    
    _pipeline.set_image_sampler(_voxel_normal_textures[0], "voxel_normals", visual_material::parameter_stage::FRAGMENT, 6, material_base::usage_type::COMBINED_IMAGE_SAMPLER);
    _pipeline.set_image_sampler(_voxel_albedo_textures[0], "voxel_albedos", visual_material::parameter_stage::FRAGMENT, 7, material_base::usage_type::COMBINED_IMAGE_SAMPLER);

    int binding_index = 8;
    int offset = 5;
    
    static const char* albedo_names[6] = {"voxel_albedos", "voxel_albedos1", "voxel_albedos2", "voxel_albedos3", "voxel_albedos4", "voxel_albedos5"};
    static const char* normal_names[6] = {"voxel_normals", "voxel_normals1","voxel_normals2", "voxel_normals3", "voxel_normals4", "voxel_normals5"};
    for( int i = 1; i < _voxel_albedo_textures.size(); ++i)
    {

        _pipeline.set_image_sampler(_voxel_albedo_textures[i], albedo_names[i], visual_material::parameter_stage::FRAGMENT, binding_index, material_base::usage_type::COMBINED_IMAGE_SAMPLER);
        _pipeline.set_image_sampler(_voxel_normal_textures[i], normal_names[i], visual_material::parameter_stage::FRAGMENT, binding_index + offset, material_base::usage_type::COMBINED_IMAGE_SAMPLER);
        
        binding_index++;
    }
    
    _pipeline.set_rendering_attachments(_swapchain->present_textures);
    _pipeline.set_offscreen_rendering(false);
    _pipeline.set_depth_enable(false);
    
    _pipeline.create();
    
    create_voxel_texture_pipelines(store);
    
    _screen_plane.create();
    
    create_command_buffers(&_offscreen_command_buffers, _device->_graphics_command_pool);
    create_command_buffers(&_voxelize_command_buffers, _device->_graphics_command_pool);
    
    for( int i = 0; i < _clear_3d_texture_command_buffers.size(); ++i)
    {
        create_command_buffers(&_clear_3d_texture_command_buffers[i], _device->_compute_command_pool);
    }
    
    for(int i = 0; i < _genered_3d_mip_maps_commands.size(); ++i)
    {
        create_command_buffers(&_genered_3d_mip_maps_commands[i], _device->_compute_command_pool);
    }

}

void deferred_renderer::create_voxel_texture_pipelines(vk::material_store& store)
{
    for( int i = 0; i < _create_voxel_mip_maps_pipelines.size(); ++i)
    {
        for( int j = 0; j < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++j)
        {
            _create_voxel_mip_maps_pipelines[i][j].set_device(_device);
            _create_voxel_mip_maps_pipelines[i][j].set_material(store.GET_MAT<compute_material>("downsize"));
        }
    }
    
    //TODO: you'll need to clear normals as well

    for( int chain_id =0; chain_id < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++chain_id)
    {
        for( int lod_id = 0; lod_id < TOTAL_LODS; ++lod_id)
        {
            _clear_voxel_texture_pipeline[lod_id][chain_id].set_image_sampler(_voxel_albedo_textures[lod_id],
                                                                   "texture_3d", 0, material_base::usage_type::STORAGE_IMAGE);
            _clear_voxel_texture_pipeline[lod_id][chain_id].commit_parameter_to_gpu(chain_id);
        }

    }
    
    for(int map_id =0; map_id < _create_voxel_mip_maps_pipelines.size(); ++map_id)
    {
        for( int chain_id = 0; chain_id < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++chain_id)
        {
            _create_voxel_mip_maps_pipelines[map_id][chain_id].set_image_sampler(_voxel_albedo_textures[map_id], "r_texture_1",
                                                            0, material_base::usage_type::COMBINED_IMAGE_SAMPLER);
            _create_voxel_mip_maps_pipelines[map_id][chain_id].set_image_sampler(_voxel_normal_textures[map_id], "r_texture_2",
                                                            1, material_base::usage_type::COMBINED_IMAGE_SAMPLER);
            _create_voxel_mip_maps_pipelines[map_id][chain_id].set_image_sampler(_voxel_albedo_textures[map_id + 1], "w_texture_1",
                                                            2, material_base::usage_type::STORAGE_IMAGE);
            _create_voxel_mip_maps_pipelines[map_id][chain_id].set_image_sampler(_voxel_normal_textures[map_id + 1], "w_texture_2",
                                                            3, material_base::usage_type::STORAGE_IMAGE);
            
            _create_voxel_mip_maps_pipelines[map_id][chain_id].commit_parameter_to_gpu(chain_id);
        }

    }
}

void deferred_renderer::setup_sampling_rays()
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


void deferred_renderer::create_semaphores_and_fences()
{
    renderer::create_semaphores_and_fences();
    
    create_semaphores(_deferred_semaphore_image_available);
    create_semaphores(_g_buffers_rendering_done);
    create_semaphores(_voxelize_semaphore);
    create_semaphores(_voxelize_semaphore_done);
    
    create_semaphores(_generate_voxel_z_axis_semaphore);
    create_semaphores(_generate_voxel_y_axis_semaphore);
    create_semaphores(_generate_voxel_x_axis_semaphore);
    
    for( int i = 0; i < _clear_voxel_textures.size(); ++i)
    {
        for( int j = 0; j < _clear_voxel_textures[i].size(); ++j)
            create_semaphore(_clear_voxel_textures[i][j]);
    }
        
    for( size_t i = 0; i < _mip_map_semaphores.size(); ++i)
    {
        create_semaphores(_mip_map_semaphores[i]);
    }
    
    for(int i = 0; i <glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++i)
    {
        create_fence(_g_buffers_fence[i]);
        create_fence(_voxelize_inflight_fence[i]);
        create_fence(_voxel_command_fence[i]);
    }
}


void deferred_renderer::record_voxelize_command_buffers(obj_shape** shapes, size_t number_of_meshes)
{
    
    _voxelize_pipeline.set_clear_attachments_colors(glm::vec4(1.0f, 1.0f, 1.0f, .0f));
    for( int i = 0; i < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++i)
    {
        _voxelize_pipeline.begin_command_recording(_voxelize_command_buffers[i], i);
        
        for( uint32_t j = 0; j < number_of_meshes; ++j)
        {
            shapes[j]->draw(_voxelize_command_buffers[i], _voxelize_pipeline, j, i);
        }
        
        _voxelize_pipeline.end_command_recording();
    }
}

void deferred_renderer::record_3d_mip_maps_commands()
{
    VkCommandBufferBeginInfo command_buffer_begin_info {};
    command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_begin_info.pInheritanceInfo = nullptr;

    assert(_genered_3d_mip_maps_commands.size() == _create_voxel_mip_maps_pipelines.size());
    
    for( int map_id = 0; map_id < TOTAL_LODS-1; ++map_id)
    {
        for( int chain_id = 0; chain_id < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++chain_id)
        {
            assert((VOXEL_CUBE_WIDTH >> map_id) % compute_pipeline::LOCAL_GROUP_SIZE == 0 && "invalid voxel cube size, voxel texture will not clear properly");
            assert((VOXEL_CUBE_HEIGHT >> map_id) % compute_pipeline::LOCAL_GROUP_SIZE == 0 && "invalid voxel cube size, voxel texture will not clear properly");
            assert((VOXEL_CUBE_DEPTH >> map_id) % compute_pipeline::LOCAL_GROUP_SIZE == 0 && "invalid voxel cube size, voxel texture will not clear properly");

            uint32_t local_groups_x = (VOXEL_CUBE_WIDTH >> map_id) / compute_pipeline::LOCAL_GROUP_SIZE;
            uint32_t local_groups_y = (VOXEL_CUBE_HEIGHT >> map_id) / compute_pipeline::LOCAL_GROUP_SIZE;
            uint32_t local_groups_z = (VOXEL_CUBE_DEPTH >> map_id) / compute_pipeline::LOCAL_GROUP_SIZE;

            if( map_id != 0)
            {
                 _create_voxel_mip_maps_pipelines[map_id][chain_id].record_begin_commands( [=]()
                 {
                     std::array<VkImageMemoryBarrier, 2> image_memory_barrier {};
                     image_memory_barrier[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                     image_memory_barrier[0].pNext = nullptr;
                     image_memory_barrier[0].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
                     image_memory_barrier[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                     image_memory_barrier[0].oldLayout = static_cast<VkImageLayout>(_voxel_albedo_textures[map_id-1][chain_id].get_image_layout());;
                     image_memory_barrier[0].newLayout = static_cast<VkImageLayout>(_voxel_albedo_textures[map_id-1][chain_id].get_image_layout());
                     image_memory_barrier[0].image = _voxel_albedo_textures[map_id-1][chain_id].get_image();
                     image_memory_barrier[0].subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
                     image_memory_barrier[0].srcQueueFamilyIndex = _device->_queue_family_indices.graphics_family.value();
                     image_memory_barrier[0].dstQueueFamilyIndex = _device->_queue_family_indices.graphics_family.value();

                     image_memory_barrier[1] = image_memory_barrier[0];
                     image_memory_barrier[1].image = _voxel_normal_textures[map_id-1][chain_id].get_image();
                     image_memory_barrier[1].oldLayout = static_cast<VkImageLayout>(_voxel_normal_textures[map_id-1][chain_id].get_image_layout());;
                     image_memory_barrier[1].newLayout = static_cast<VkImageLayout>(_voxel_normal_textures[map_id-1][chain_id].get_image_layout());
                     constexpr uint32_t VK_FLAGS_NONE = 0;

                     vkCmdPipelineBarrier(
                                          _genered_3d_mip_maps_commands[map_id][chain_id],
                                          VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                          VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
                                          VK_FLAGS_NONE,
                                          0, nullptr,
                                          0, nullptr,
                                          image_memory_barrier.size(), image_memory_barrier.data());
                 } );
            }
            
            _create_voxel_mip_maps_pipelines[map_id][chain_id].record_dispatch_commands(_genered_3d_mip_maps_commands[map_id][chain_id],
                                                            local_groups_x, local_groups_y, local_groups_z, chain_id);
        }
    }
}

void deferred_renderer::record_clear_texture_3d_buffer()
{
    VkCommandBufferBeginInfo command_buffer_begin_info {};
    command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_begin_info.pInheritanceInfo = nullptr;
    
    for( int lod_id = 0; lod_id < _clear_3d_texture_command_buffers.size(); ++lod_id)
    {
        for( int j = 0; j < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++j)
        {
            assert((VOXEL_CUBE_WIDTH >> lod_id) % compute_pipeline::LOCAL_GROUP_SIZE == 0 && "invalid voxel cube size, voxel texture will not clear properly");
            assert((VOXEL_CUBE_HEIGHT >> lod_id) % compute_pipeline::LOCAL_GROUP_SIZE == 0 && "invalid voxel cube size, voxel texture will not clear properly");
            assert((VOXEL_CUBE_DEPTH >> lod_id) % compute_pipeline::LOCAL_GROUP_SIZE == 0 && "invalid voxel cube size, voxel texture will not clear properly");
            
            uint32_t local_groups_x = (VOXEL_CUBE_WIDTH >> lod_id) / compute_pipeline::LOCAL_GROUP_SIZE;
            uint32_t local_groups_y = (VOXEL_CUBE_HEIGHT >> lod_id) / compute_pipeline::LOCAL_GROUP_SIZE;
            uint32_t local_groups_z = (VOXEL_CUBE_DEPTH >> lod_id) / compute_pipeline::LOCAL_GROUP_SIZE;
            
            _clear_voxel_texture_pipeline[lod_id][j].record_dispatch_commands(_clear_3d_texture_command_buffers[lod_id][j],
                                                                local_groups_x, local_groups_y, local_groups_z, j);
        }
    }
}

void deferred_renderer::record_command_buffers(obj_shape** shapes, size_t number_of_shapes)
{
    
    _mrt_pipeline.set_clear_attachments_colors(glm::vec4(0.f));
    _mrt_pipeline.set_clear_depth(glm::vec2(1.0f, 0.0f));
    
    for (uint32_t i = 0; i < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; i++)
    {
        
        _mrt_pipeline.begin_command_recording(_offscreen_command_buffers[i], i);
        for( uint32_t j = 0; j < number_of_shapes; ++j)
        {
            shapes[j]->draw(_offscreen_command_buffers[i], _mrt_pipeline, j, i);
        }
        
        _mrt_pipeline.end_command_recording();
    }
    
    record_voxelize_command_buffers(shapes, number_of_shapes);
    record_clear_texture_3d_buffer();
    record_3d_mip_maps_commands();
    
    std::array<obj_shape*, 1> screen_plane_array = { &_screen_plane };
    renderer::record_command_buffers(screen_plane_array.data(), screen_plane_array.size());
}

void deferred_renderer::perform_final_drawing_setup()
{
    if( !_setup_initialized)
    {
        _setup_initialized = true;
        record_command_buffers(_shapes.data(), _shapes.size());
        _pipeline_created = true;
    }
    
    for( int i = 0; i < _shapes.size(); ++i)
    {
        _voxelize_pipeline.get_dynamic_parameters(vk::visual_material::parameter_stage::VERTEX, 3, _deferred_image_index)[i]["model"] = _shapes[i]->transform.get_transform_matrix();
    }
    
    _voxelize_pipeline.commit_parameters_to_gpu(_deferred_image_index);
    _mrt_pipeline.commit_parameters_to_gpu(_deferred_image_index);
    _pipeline.commit_parameters_to_gpu(_deferred_image_index);
}

void deferred_renderer::clear_voxels_textures()
{
    std::array<VkCommandBuffer, TOTAL_LODS> clear_commands {};
    std::array<VkPipelineStageFlags, TOTAL_LODS> wait_stage_mask= { };
    
    for( int i =0; i < _clear_3d_texture_command_buffers.size(); ++i)
    {
        clear_commands[i] = _clear_3d_texture_command_buffers[i][_deferred_image_index];
        wait_stage_mask[i] = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    }
    
    //The clearing of voxels happens in parallel
    VkSubmitInfo submit_info = {};
    submit_info.commandBufferCount = clear_commands.size();
    submit_info.pCommandBuffers = clear_commands.data();
    submit_info.waitSemaphoreCount = 0;
    submit_info.pWaitDstStageMask = wait_stage_mask.data();
    submit_info.pWaitSemaphores = nullptr;
    submit_info.pSignalSemaphores = _clear_voxel_textures[_deferred_image_index].data();
    submit_info.signalSemaphoreCount = (uint32_t)_clear_voxel_textures[_deferred_image_index].size();
    
    VkResult result = vkQueueSubmit(_device->_compute_queue, 1, &submit_info, nullptr);
    
    ASSERT_VULKAN(result);
}

void deferred_renderer::generate_voxel_mip_maps(VkSemaphore& semaphore)
{
    //TODO: batch these into one submit...
    for( int i = 0; i < _genered_3d_mip_maps_commands.size(); ++i)
    {
        std::array<VkPipelineStageFlags, 1> wait_stage_mask= { VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT };
        VkSubmitInfo submit_info = {};
        submit_info.commandBufferCount = 1;
        submit_info.pWaitDstStageMask = wait_stage_mask.data();
        submit_info.pCommandBuffers = &_genered_3d_mip_maps_commands[i][_deferred_image_index];
        submit_info.waitSemaphoreCount =  1;
        submit_info.pWaitSemaphores = i == 0 ? &semaphore : &_mip_map_semaphores[_deferred_image_index][i-1];
        submit_info.pSignalSemaphores = &_mip_map_semaphores[_deferred_image_index][i];
        submit_info.signalSemaphoreCount = 1;
        
        VkResult result = vkQueueSubmit(_device->_compute_queue, 1, &submit_info, nullptr);
        ASSERT_VULKAN(result);
    }
}

void deferred_renderer::generate_voxel_textures(vk::camera &camera)
{
    vk::shader_parameter::shader_params_group& voxelize_vertex_params = _voxelize_pipeline.get_uniform_parameters(vk::visual_material::parameter_stage::VERTEX, 0, _deferred_image_index);
    vk::shader_parameter::shader_params_group& voxelize_frag_params = _voxelize_pipeline.get_uniform_parameters(vk::visual_material::parameter_stage::FRAGMENT, 2, _deferred_image_index);
    vk::shader_parameter::shader_params_group& deferred_output_params = _pipeline.get_uniform_parameters(vk::visual_material::parameter_stage::FRAGMENT, 5, _deferred_image_index);
    
    voxelize_frag_params["voxel_coords"] = glm::vec3( static_cast<float>(VOXEL_CUBE_WIDTH), static_cast<float>(VOXEL_CUBE_HEIGHT), static_cast<float>(VOXEL_CUBE_DEPTH));
    deferred_output_params["eye_inverse_view_matrix"] = glm::inverse(camera.view_matrix);
    clear_voxels_textures();

    constexpr float distance = 8.f;
    std::array<glm::vec3, 3> cam_positions = {  glm::vec3(0.0f, 0.0f, -distance),glm::vec3(0.0f, distance, 0.0f), glm::vec3(distance, 0.0f, 0.0f)};
    std::array<glm::vec3, 3> up_vectors = { glm::vec3 {0.0f, 1.0f, 0.0f}, glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)};
    std::array<VkSemaphore, 3> semaphores = { _generate_voxel_z_axis_semaphore[_deferred_image_index],
        _generate_voxel_y_axis_semaphore[_deferred_image_index], _generate_voxel_x_axis_semaphore[_deferred_image_index]};
    
    glm::mat4 project_to_voxel_screen = glm::mat4(1.0f);

    //voxelize, we are going to build the voxel texture from cam_positions.size() views.
    std::array<VkSubmitInfo, 3> submits {};
    
    size_t i = 0;
    for( ; i < 3; ++i)
    {
        _ortho_camera.position = cam_positions[i];
        _ortho_camera.forward = -_ortho_camera.position;
        
        _ortho_camera.up = up_vectors[i];
        _ortho_camera.update_view_matrix();
        
        if( i == 0)
        {
            deferred_output_params["vox_view_projection"] = _ortho_camera.get_projection_matrix() * _ortho_camera.view_matrix;
            deferred_output_params["eye_in_world_space"] = camera.position;
            project_to_voxel_screen = _ortho_camera.get_projection_matrix() * _ortho_camera.view_matrix;
            voxelize_frag_params["project_to_voxel_screen"] = project_to_voxel_screen;
        }
        
        voxelize_frag_params["inverse_view_projection"] = glm::inverse( _ortho_camera.get_projection_matrix() * _ortho_camera.view_matrix);
        voxelize_vertex_params["view"] = _ortho_camera.view_matrix;
        voxelize_vertex_params["projection"] =_ortho_camera.get_projection_matrix();

        voxelize_vertex_params["light_position"] = _light_pos;
        voxelize_vertex_params["eye_position"] = camera.position;
        
        _voxelize_pipeline.commit_parameters_to_gpu(_deferred_image_index);
        
        std::array<VkPipelineStageFlags, 1> wait_stage_mask= {};
        
        submits[i].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submits[i].commandBufferCount = 1;
        submits[i].pWaitDstStageMask = &wait_stage_mask[0];
        submits[i].waitSemaphoreCount = i != 0;
        submits[i].pWaitSemaphores = i == 0 ? &_clear_voxel_textures[_deferred_image_index][0] : &semaphores[i-1];
        submits[i].pCommandBuffers = &(_voxelize_command_buffers[_deferred_image_index]);
        submits[i].signalSemaphoreCount = 1;
        submits[i].pSignalSemaphores =  &semaphores[i];
        
        
        //TODO: the following fence in theory is not necessary, but removing it causes some of the renderings to not be written to the texture, even though I have
        //semphores in the submissions, I even tried barriers, and changed the moltenVK implementations of the semaphore to fence and events, nothing made the bug go away.
        //Do not bulk these submits into due to this bug.  It may be either in the OS or MoltenVK, am not sure, or somewhere in this code, I have no clue.
        vkResetFences(_device->_logical_device, 1, &_voxelize_inflight_fence[_deferred_image_index]);
        vkQueueSubmit(_device->_graphics_queue,1, &submits[i], _voxelize_inflight_fence[_deferred_image_index]);
        vkWaitForFences(_device->_logical_device, 1, &_voxelize_inflight_fence[_deferred_image_index], VK_TRUE, std::numeric_limits<uint64_t>::max());
    }

    generate_voxel_mip_maps(semaphores[i-1]);
}


void deferred_renderer::draw(camera& camera)
{
    vkAcquireNextImageKHR(_device->_logical_device, _swapchain->get_vk_swapchain(),
                          std::numeric_limits<uint64_t>::max(), _semaphore_image_available, VK_NULL_HANDLE, &_deferred_image_index);
    
    vk::shader_parameter::shader_params_group& display_fragment_params = _pipeline.get_uniform_parameters(vk::visual_material::parameter_stage::FRAGMENT, 5, _deferred_image_index) ;
    
    display_fragment_params["world_cam_position"] = glm::vec4(camera.position, 1.0f);
    display_fragment_params["world_light_position"] = _light_pos;
    display_fragment_params["light_color"] = _light_color;
    display_fragment_params["mode"] = static_cast<int>(_rendering_mode);
    
    int binding = 0;
    vk::shader_parameter::shader_params_group& display_params =   _pipeline.get_uniform_parameters(vk::visual_material::parameter_stage::VERTEX, binding, _deferred_image_index);
    
    display_params["width"] = static_cast<float>(_swapchain->get_vk_swap_extent().width);
    display_params["height"] = static_cast<float>(_swapchain->get_vk_swap_extent().height);
    perform_final_drawing_setup();
    
    if(_deferred_image_index % 2 != 0)
        generate_voxel_textures(camera);
    
    std::array<VkSemaphore, 2> wait_semaphores{ _mip_map_semaphores[_deferred_image_index][ _mip_map_semaphores.size()-1 ],
        _semaphore_image_available};
    //render g-buffers
    VkResult result = {};
    std::array<VkSubmitInfo,3> submit_info = {};
    submit_info[0].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info[0].pNext = nullptr;
    submit_info[0].waitSemaphoreCount = wait_semaphores.size();
    submit_info[0].pWaitSemaphores = wait_semaphores.data();
    VkPipelineStageFlags wait_stage_mask[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submit_info[0].pWaitDstStageMask = wait_stage_mask;
    submit_info[0].commandBufferCount = 1;
    submit_info[0].pCommandBuffers = &(_offscreen_command_buffers[_deferred_image_index]);
    submit_info[0].signalSemaphoreCount = 1;
    submit_info[0].pSignalSemaphores = &_g_buffers_rendering_done[_deferred_image_index];
    
    //render scene with g buffers and 3d voxel texture
    submit_info[1].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info[1].pNext = nullptr;
    submit_info[1].waitSemaphoreCount = 1;
    submit_info[1].pWaitSemaphores = &_g_buffers_rendering_done[_deferred_image_index];
    submit_info[1].pWaitDstStageMask = wait_stage_mask;
    submit_info[1].commandBufferCount = 1;
    submit_info[1].pCommandBuffers = &(_command_buffers[_deferred_image_index]);
    submit_info[1].signalSemaphoreCount = 1;
    submit_info[1].pSignalSemaphores = &_semaphore_rendering_done;

    result = vkQueueSubmit(_device->_graphics_queue, submit_info.size(), submit_info.data(), nullptr);

    ASSERT_VULKAN(result);
    //present the scene to viewer
    VkPresentInfoKHR present_info {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pNext = nullptr;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores =&_semaphore_rendering_done;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &_swapchain->get_vk_swapchain();
    present_info.pImageIndices = &_deferred_image_index;
    present_info.pResults = nullptr;
    result = vkQueuePresentKHR(_device->_present_queue, &present_info);
    //TODO: check to see if you can collapse the 3 vkQueueSubmit calls into one, per nvidia: https://devblogs.nvidia.com/vulkan-dos-donts/
    
    ASSERT_VULKAN(result);

}

void deferred_renderer::destroy()
{
    renderer::destroy();
    for(int i = 0; i < _deferred_semaphore_image_available.size(); ++i)
    {
        vkDestroySemaphore(_device->_logical_device, _deferred_semaphore_image_available[i], nullptr);
        vkDestroySemaphore(_device->_logical_device, _g_buffers_rendering_done[i], nullptr);
        vkDestroySemaphore(_device->_logical_device, _voxelize_semaphore[i], nullptr);
        vkDestroySemaphore(_device->_logical_device, _voxelize_semaphore_done[i], nullptr);

        vkDestroySemaphore(_device->_logical_device, _generate_voxel_x_axis_semaphore[i], nullptr);
        vkDestroySemaphore(_device->_logical_device, _generate_voxel_y_axis_semaphore[i], nullptr);
        vkDestroySemaphore(_device->_logical_device, _generate_voxel_z_axis_semaphore[i], nullptr);
    }
    
    for( int i = 0; i < _clear_voxel_textures.size(); ++i)
    {
        for(int j =0; j < _clear_voxel_textures[i].size(); ++j)
        {
            vkDestroySemaphore(_device->_logical_device, _clear_voxel_textures[i][j], nullptr);
        }
    }

    
    for( size_t i = 0; i < _mip_map_semaphores.size(); ++i)
    {
        for(int j = 0; j < _mip_map_semaphores[i].size(); ++j)
        {
            vkDestroySemaphore(_device->_logical_device, _mip_map_semaphores[i][j], nullptr);
        }
    }
    
    vkFreeCommandBuffers(_device->_logical_device, _device->_graphics_command_pool,
                         glfw_swapchain::NUM_SWAPCHAIN_IMAGES, _offscreen_command_buffers);
    
    vkFreeCommandBuffers(_device->_logical_device, _device->_compute_command_pool,
                         glfw_swapchain::NUM_SWAPCHAIN_IMAGES, _voxelize_command_buffers);
  
    for(int i = 0; i < _clear_3d_texture_command_buffers.size(); ++i)
    {
        vkFreeCommandBuffers(_device->_logical_device, _device->_compute_command_pool,
                             glfw_swapchain::NUM_SWAPCHAIN_IMAGES, _clear_3d_texture_command_buffers[i]) ;
    }

    for(int i = 0; i < _genered_3d_mip_maps_commands.size(); ++i)
    {
        vkFreeCommandBuffers(_device->_logical_device, _device->_compute_command_pool,
                             glfw_swapchain::NUM_SWAPCHAIN_IMAGES, _genered_3d_mip_maps_commands[i]) ;
    }

    
    delete[] _offscreen_command_buffers;
    
    for (size_t i = 0; i < vk::glfw_swapchain::NUM_SWAPCHAIN_IMAGES; i++)
    {
        vkDestroyFence(_device->_logical_device, _g_buffers_fence[i], nullptr);
        vkDestroyFence(_device->_logical_device, _voxelize_inflight_fence[i], nullptr);
        _g_buffers_fence[i] = VK_NULL_HANDLE;
    }
    
    _mrt_pipeline.destroy();
    _voxelize_pipeline.destroy();
    
    for( size_t i  = 0; i < _voxel_normal_textures.size(); ++i)
    {
        for(size_t j = 0; j < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++j)
        {
            _voxel_albedo_textures[i][j].destroy();
            _voxel_normal_textures[i][j].destroy();
        }

    }

    for( int i = 0; i < _clear_voxel_texture_pipeline.size(); ++i)
    {
        for(size_t j = 0; j < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++j)
        {
            _clear_voxel_texture_pipeline[i][j].destroy();
        }
        
    }
    
    for( size_t i = 0; i < _create_voxel_mip_maps_pipelines.size(); ++i)
    {
        for(size_t j = 0; j < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++j)
        {
            _create_voxel_mip_maps_pipelines[i][j].destroy();

        }
    }
}
