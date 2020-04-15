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
#include "render_pass.h"
#include "graph.h"
#include "compute_node.h"
#include "graphics_node.h"



using namespace vk;

deferred_renderer::deferred_renderer(device* device, GLFWwindow* window, vk::glfw_swapchain* swapchain, vk::material_store& store, std::vector<obj_shape*>& shapes):
renderer(device, window, swapchain, store, "deferred_output"),
_screen_plane(device),
_mrt_render_pass(device, glm::vec2(swapchain->get_vk_swap_extent().width, swapchain->get_vk_swap_extent().height)),
_voxelize_render_pass(device, glm::vec2(VOXEL_CUBE_WIDTH, VOXEL_CUBE_HEIGHT) ),
_ortho_camera(_voxel_world_dimensions.x, _voxel_world_dimensions.y, _voxel_world_dimensions.z)
{
    attachment_group<4>& mrt_attachment_group = _mrt_render_pass.get_attachment_group();
    for(int i = 0; i < _g_buffer_textures.size(); ++i)
    {
        mrt_attachment_group.add_attachment(_g_buffer_textures[i]);
        mrt_attachment_group.set_filter(i, image::filter::NEAREST);
        
        if( buffer_ids::NORMALS_ATTACHMENT_ID == i)
            mrt_attachment_group.set_format(i, image::formats::R8G8_SIGNED_NORMALIZED);
    }
    
    mrt_attachment_group.add_attachment(_swapchain->present_textures);
    
    attachment_group<1>& voxel_attachment_group = _voxelize_render_pass.get_attachment_group();
    voxel_attachment_group.add_attachment(_voxel_2d_view[0]);


    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    
    vkCreateFence(_device->_logical_device, &fenceInfo, nullptr, &_fence);
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
    
    setup_sampling_rays();
    
    mrt_render_pass::subpass_s& mrt_subpass = _mrt_render_pass.add_subpass(store, "mrt");
    mrt_render_pass::subpass_s& subpass = _mrt_render_pass.add_subpass(store, "deferred_output");
    
    for( int i = 0; i < shapes.size(); ++i)
    {
        _shapes.push_back(shapes[i]);
        _mrt_render_pass.add_object(*shapes[i]);
        _voxelize_render_pass.add_object(*shapes[i]);
        _mrt_render_pass.skip_subpass(shapes[i], 1);
        
    }
    _mrt_render_pass.add_object(_screen_plane);
    
    _mrt_render_pass.skip_subpass(&_screen_plane, 0);
    
    mrt_subpass.add_output_attachment(NORMALS_ATTACHMENT_ID);
    mrt_subpass.add_output_attachment(ALBEDOS_ATTACHMENT_ID );
    mrt_subpass.add_output_attachment(POSITIONS_ATTACHMENT_ID);
    mrt_subpass.add_output_attachment(DEPTH_ATTACHMENT_ID);


    int binding = 0;
    mrt_subpass.init_parameter("view", visual_material::parameter_stage::VERTEX, glm::mat4(0), binding);
    mrt_subpass.init_parameter("projection", visual_material::parameter_stage::VERTEX, glm::mat4(0), binding);
    mrt_subpass.init_parameter("lightPosition", visual_material::parameter_stage::VERTEX, glm::vec3(0), binding);

    //TODO: set_number_of_blend_attachments function could now go away...
    mrt_subpass.set_number_of_blend_attachments(3);
    mrt_subpass.modify_attachment_blend(NORMALS_ATTACHMENT_ID, mrt_render_pass::write_channels::RGBA, false);
    mrt_subpass.modify_attachment_blend(ALBEDOS_ATTACHMENT_ID, mrt_render_pass::write_channels::RGBA, false);
    mrt_subpass.modify_attachment_blend(POSITIONS_ATTACHMENT_ID, mrt_render_pass::write_channels::RGBA, false);
    
    glm::mat4 identity = glm::mat4(1);
    mrt_subpass.init_dynamic_params("model", visual_material::parameter_stage::VERTEX, identity, shapes.size(), 1);
    
    voxelize_render_pass::subpass_s& voxelize_subpass = _voxelize_render_pass.add_subpass(store, "voxelizer");
    
    enum{ VOXEL_ATTACHMENT_ID = 0 };
    voxelize_subpass.add_output_attachment(VOXEL_ATTACHMENT_ID);
    
    
    voxelize_subpass.set_number_of_blend_attachments(1);
    voxelize_subpass.modify_attachment_blend(0, voxelize_render_pass::write_channels::RGBA, false);
    
    voxelize_subpass.set_image_sampler(_voxel_albedo_textures[0], "voxel_albedo_texture",
                                         visual_material::parameter_stage::FRAGMENT, 1, resource::usage_type::STORAGE_IMAGE);
    voxelize_subpass.set_image_sampler(_voxel_normal_textures[0], "voxel_normal_texture",
                                         visual_material::parameter_stage::FRAGMENT, 4, resource::usage_type::STORAGE_IMAGE);
    
    voxelize_subpass.init_parameter("inverse_view_projection", visual_material::parameter_stage::FRAGMENT, glm::mat4(1.0f), 2);
    voxelize_subpass.init_parameter("project_to_voxel_screen", visual_material::parameter_stage::FRAGMENT, glm::mat4(1.0f), 2);
    voxelize_subpass.init_parameter("voxel_coords", visual_material::parameter_stage::FRAGMENT, glm::vec3(1.0f), 2);
    
    voxelize_subpass.init_parameter("view", visual_material::parameter_stage::VERTEX, glm::mat4(1.0f), 0);
    voxelize_subpass.init_parameter("projection", visual_material::parameter_stage::VERTEX, glm::mat4(1.0f), 0);
    voxelize_subpass.init_parameter("light_position", visual_material::parameter_stage::VERTEX, glm::vec3(1.0f), 0);
    voxelize_subpass.init_parameter("eye_position", visual_material::parameter_stage::VERTEX, glm::vec3(1.0f), 0);
    voxelize_subpass.init_dynamic_params("model", visual_material::parameter_stage::VERTEX, identity, shapes.size(), 3);
    
    voxelize_subpass.set_cull_mode( voxelize_render_pass::graphics_pipeline_type::cull_mode::NONE);


    //composite subpass for deferred rendering

    subpass.add_input_attachment( "normals", NORMALS_ATTACHMENT_ID, visual_material::parameter_stage::FRAGMENT, 1 );
    subpass.add_input_attachment("albedos", ALBEDOS_ATTACHMENT_ID, visual_material::parameter_stage::FRAGMENT, 2);
    subpass.add_input_attachment("positions", POSITIONS_ATTACHMENT_ID, visual_material::parameter_stage::FRAGMENT, 3);

    subpass.add_input_attachment("depth", DEPTH_ATTACHMENT_ID, visual_material::parameter_stage::FRAGMENT, 4);
    
    subpass.add_output_attachment(PRESENT_ATTACHMENT_ID);
    
    subpass.init_parameter("width", visual_material::parameter_stage::VERTEX, 0.f, 0);
    subpass.init_parameter("height", visual_material::parameter_stage::VERTEX, 0.f, 0);
    
    subpass.set_image_sampler(_voxel_normal_textures[0], "voxel_normals", visual_material::parameter_stage::FRAGMENT, 6, material_base::usage_type::COMBINED_IMAGE_SAMPLER);
    subpass.set_image_sampler(_voxel_albedo_textures[0], "voxel_albedos", visual_material::parameter_stage::FRAGMENT, 7, material_base::usage_type::COMBINED_IMAGE_SAMPLER);
    
    glm::vec4 world_scale_voxel = glm::vec4(float(_voxel_world_dimensions.x/VOXEL_CUBE_WIDTH),
                                            float(_voxel_world_dimensions.y/VOXEL_CUBE_HEIGHT),
                                            float(_voxel_world_dimensions.z/VOXEL_CUBE_DEPTH), 1.0f);
    
    subpass.init_parameter("world_cam_position", visual_material::parameter_stage::FRAGMENT, glm::vec4(0.0f), 5);
    subpass.init_parameter("world_light_position", visual_material::parameter_stage::FRAGMENT, glm::vec3(0.0f), 5);
    subpass.init_parameter("light_color", visual_material::parameter_stage::FRAGMENT, glm::vec4(0.0f), 5);
    subpass.init_parameter("voxel_size_in_world_space", visual_material::parameter_stage::FRAGMENT, world_scale_voxel, 5);
    subpass.init_parameter("mode", visual_material::parameter_stage::FRAGMENT, int(0), 5);
    subpass.init_parameter("sampling_rays", visual_material::parameter_stage::FRAGMENT, _sampling_rays.data(), _sampling_rays.size(), 5);
    subpass.init_parameter("vox_view_projection", visual_material::parameter_stage::FRAGMENT, glm::mat4(1.0f), 5);
    subpass.init_parameter("num_of_lods", visual_material::parameter_stage::FRAGMENT, int(TOTAL_LODS), 5);
    subpass.init_parameter("eye_in_world_space", visual_material::parameter_stage::FRAGMENT, glm::vec3(0), 5);
    subpass.init_parameter("eye_inverse_view_matrix", visual_material::parameter_stage::FRAGMENT, glm::mat4(1.0f), 5);
    
    subpass.set_image_sampler(_voxel_normal_textures[0], "voxel_normals", visual_material::parameter_stage::FRAGMENT, 6, material_base::usage_type::COMBINED_IMAGE_SAMPLER);
    subpass.set_image_sampler(_voxel_albedo_textures[0], "voxel_albedos", visual_material::parameter_stage::FRAGMENT, 7, material_base::usage_type::COMBINED_IMAGE_SAMPLER);

    _screen_plane.create();
    
    int binding_index = 8;
    int offset = 5;
    
    static const char* albedo_names[6] = {"voxel_albedos", "voxel_albedos1", "voxel_albedos2", "voxel_albedos3", "voxel_albedos4", "voxel_albedos5"};
    static const char* normal_names[6] = {"voxel_normals", "voxel_normals1","voxel_normals2", "voxel_normals3", "voxel_normals4", "voxel_normals5"};
    for( int i = 1; i < _voxel_albedo_textures.size(); ++i)
    {

        subpass.set_image_sampler(_voxel_albedo_textures[i], albedo_names[i], visual_material::parameter_stage::FRAGMENT, binding_index, material_base::usage_type::COMBINED_IMAGE_SAMPLER);
        subpass.set_image_sampler(_voxel_normal_textures[i], normal_names[i], visual_material::parameter_stage::FRAGMENT, binding_index + offset, material_base::usage_type::COMBINED_IMAGE_SAMPLER);
        
        binding_index++;
    }
    
    for( int i = 0; i < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++i)
    {
        _mrt_render_pass.create(i);
        _voxelize_render_pass.create(i);
    }
        
    create_voxel_texture_pipelines(store);
    

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
            _clear_voxel_texture_pipeline[lod_id][chain_id].set_image_sampler(_voxel_albedo_textures[lod_id][chain_id],
                                                                   "texture_3d", 0);
            _clear_voxel_texture_pipeline[lod_id][chain_id].commit_parameter_to_gpu();
        }

    }
    
    for(int map_id =0; map_id < _create_voxel_mip_maps_pipelines.size(); ++map_id)
    {
        for( int chain_id = 0; chain_id < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++chain_id)
        {
            //Note: per sascha willems compute example, compute shaders use storage_image format for reads and writes
            //if you try a combined sampler here, validation layers will throw errrors.  Here is example from willems:
            //https://github.com/SaschaWillems/Vulkan/blob/master/examples/computeshader/computeshader.cpp
            
            _create_voxel_mip_maps_pipelines[map_id][chain_id].set_image_sampler(_voxel_albedo_textures[map_id][chain_id], "r_texture_1",
                                                            0);
            _create_voxel_mip_maps_pipelines[map_id][chain_id].set_image_sampler(_voxel_normal_textures[map_id][chain_id], "r_texture_2",
                                                            1);
            _create_voxel_mip_maps_pipelines[map_id][chain_id].set_image_sampler(_voxel_albedo_textures[map_id + 1][chain_id], "w_texture_1",
                                                            2);
            _create_voxel_mip_maps_pipelines[map_id][chain_id].set_image_sampler(_voxel_normal_textures[map_id + 1][chain_id], "w_texture_2",
                                                            3);
            
            _create_voxel_mip_maps_pipelines[map_id][chain_id].commit_parameter_to_gpu();
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


void deferred_renderer::record_voxelize_command_buffers(VkCommandBuffer& buffer, uint32_t swapchain_id)
{
    
    _voxelize_render_pass.set_clear_attachments_colors(glm::vec4(1.0f, 1.0f, 1.0f, .0f));

    _voxelize_render_pass.record_draw_commands(buffer, swapchain_id);

}

void deferred_renderer::record_3d_mip_maps_commands(uint32_t swapchain_id)
{
    VkCommandBufferBeginInfo command_buffer_begin_info {};
    command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_begin_info.pInheritanceInfo = nullptr;

    assert(_genered_3d_mip_maps_commands.size() == _create_voxel_mip_maps_pipelines.size());
    
    for( int map_id = 0; map_id < TOTAL_LODS-1; ++map_id)
    {
        
        assert((VOXEL_CUBE_WIDTH >> map_id) % compute_pipeline::LOCAL_GROUP_SIZE == 0 && "invalid voxel cube size, voxel texture will not clear properly");
        assert((VOXEL_CUBE_HEIGHT >> map_id) % compute_pipeline::LOCAL_GROUP_SIZE == 0 && "invalid voxel cube size, voxel texture will not clear properly");
        assert((VOXEL_CUBE_DEPTH >> map_id) % compute_pipeline::LOCAL_GROUP_SIZE == 0 && "invalid voxel cube size, voxel texture will not clear properly");

        uint32_t local_groups_x = (VOXEL_CUBE_WIDTH >> map_id) / compute_pipeline::LOCAL_GROUP_SIZE;
        uint32_t local_groups_y = (VOXEL_CUBE_HEIGHT >> map_id) / compute_pipeline::LOCAL_GROUP_SIZE;
        uint32_t local_groups_z = (VOXEL_CUBE_DEPTH >> map_id) / compute_pipeline::LOCAL_GROUP_SIZE;

        if( map_id != 0)
        {
             _create_voxel_mip_maps_pipelines[map_id][swapchain_id].record_begin_commands( [&]()
             {
                 std::array<VkImageMemoryBarrier, 2> image_memory_barrier {};
                 image_memory_barrier[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                 image_memory_barrier[0].pNext = nullptr;
                 image_memory_barrier[0].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
                 image_memory_barrier[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                 image_memory_barrier[0].oldLayout = static_cast<VkImageLayout>(_voxel_albedo_textures[map_id-1][swapchain_id].get_native_layout());;
                 image_memory_barrier[0].newLayout = static_cast<VkImageLayout>(_voxel_albedo_textures[map_id-1][swapchain_id].get_native_layout());
                 image_memory_barrier[0].image = _voxel_albedo_textures[map_id-1][swapchain_id].get_image();
                 image_memory_barrier[0].subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
                 image_memory_barrier[0].srcQueueFamilyIndex = _device->_queue_family_indices.graphics_family.value();
                 image_memory_barrier[0].dstQueueFamilyIndex = _device->_queue_family_indices.graphics_family.value();

                 image_memory_barrier[1] = image_memory_barrier[0];
                 image_memory_barrier[1].image = _voxel_normal_textures[map_id-1][swapchain_id].get_image();
                 image_memory_barrier[1].oldLayout = static_cast<VkImageLayout>(_voxel_normal_textures[map_id-1][swapchain_id].get_native_layout());;
                 image_memory_barrier[1].newLayout = static_cast<VkImageLayout>(_voxel_normal_textures[map_id-1][swapchain_id].get_native_layout());
                 constexpr uint32_t VK_FLAGS_NONE = 0;

                 vkCmdPipelineBarrier(
                                      _genered_3d_mip_maps_commands[map_id][swapchain_id],
                                      VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                      VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
                                      VK_FLAGS_NONE,
                                      0, nullptr,
                                      0, nullptr,
                                      image_memory_barrier.size(), image_memory_barrier.data());
             } );
        }
            
        _create_voxel_mip_maps_pipelines[map_id][swapchain_id].record_dispatch_commands(_genered_3d_mip_maps_commands[map_id][swapchain_id],
                                                            local_groups_x, local_groups_y, local_groups_z);
        
    }
}

void deferred_renderer::record_clear_texture_3d_buffer( uint32_t swapchain_id)
{
    VkCommandBufferBeginInfo command_buffer_begin_info {};
    command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_begin_info.pInheritanceInfo = nullptr;
    
    for( int lod_id = 0; lod_id < _clear_3d_texture_command_buffers.size(); ++lod_id)
    {

        assert((VOXEL_CUBE_WIDTH >> lod_id) % compute_pipeline::LOCAL_GROUP_SIZE == 0 && "invalid voxel cube size, voxel texture will not clear properly");
        assert((VOXEL_CUBE_HEIGHT >> lod_id) % compute_pipeline::LOCAL_GROUP_SIZE == 0 && "invalid voxel cube size, voxel texture will not clear properly");
        assert((VOXEL_CUBE_DEPTH >> lod_id) % compute_pipeline::LOCAL_GROUP_SIZE == 0 && "invalid voxel cube size, voxel texture will not clear properly");
        
        uint32_t local_groups_x = (VOXEL_CUBE_WIDTH >> lod_id) / compute_pipeline::LOCAL_GROUP_SIZE;
        uint32_t local_groups_y = (VOXEL_CUBE_HEIGHT >> lod_id) / compute_pipeline::LOCAL_GROUP_SIZE;
        uint32_t local_groups_z = (VOXEL_CUBE_DEPTH >> lod_id) / compute_pipeline::LOCAL_GROUP_SIZE;
        
        _clear_voxel_texture_pipeline[lod_id][swapchain_id].record_dispatch_commands(_clear_3d_texture_command_buffers[lod_id][swapchain_id],
                                                            local_groups_x, local_groups_y, local_groups_z);
    }
}

void deferred_renderer::record_command_buffers(VkCommandBuffer& buffer, uint32_t swapchain_id)
{
    
    _mrt_render_pass.set_clear_attachments_colors(glm::vec4(0.f));
    _mrt_render_pass.set_clear_depth(glm::vec2(1.0f, 0.0f));
        
    _mrt_render_pass.record_draw_commands(_offscreen_command_buffers[swapchain_id], swapchain_id);
    
    record_voxelize_command_buffers(_voxelize_command_buffers[swapchain_id], swapchain_id);
    record_clear_texture_3d_buffer(swapchain_id);
    record_3d_mip_maps_commands(swapchain_id);
}

void deferred_renderer::perform_final_drawing_setup(VkCommandBuffer& buffer, uint32_t swapchain_id)
{
    if( !_setup_initialized[_deferred_image_index])
    {
        _setup_initialized[_deferred_image_index] = true;
        record_command_buffers(buffer, _deferred_image_index);
        _pipeline_created[_deferred_image_index] = true;
    }
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
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = clear_commands.size();
    submit_info.pCommandBuffers = clear_commands.data();
    submit_info.waitSemaphoreCount = 0;
    submit_info.pWaitSemaphores = nullptr;
    submit_info.pWaitDstStageMask = wait_stage_mask.data();
    submit_info.pSignalSemaphores = &_generate_voxel_z_axis_semaphore[_deferred_image_index];
    submit_info.signalSemaphoreCount = 1;

    VkResult result = vkQueueSubmit(_device->_compute_queue, 1, &submit_info, nullptr);
    
    ASSERT_VULKAN(result);
}

void deferred_renderer::generate_voxel_mip_maps()
{
    //TODO: batch these into one submit...
    for( int i = 0; i < _genered_3d_mip_maps_commands.size(); ++i)
    {
        std::array<VkPipelineStageFlags, 1> wait_stage_mask= { VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT };
        VkSubmitInfo submit_info = {};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pWaitDstStageMask = wait_stage_mask.data();
        submit_info.pCommandBuffers = &_genered_3d_mip_maps_commands[i][_deferred_image_index];
        submit_info.waitSemaphoreCount =  i == 0 ? 0 : 1;
        submit_info.pWaitSemaphores = i == 0 ? nullptr : &_mip_map_semaphores[i-1][_deferred_image_index];
        submit_info.pSignalSemaphores = &_mip_map_semaphores[i][_deferred_image_index];
        submit_info.signalSemaphoreCount = 1;
        
        VkResult result = vkQueueSubmit(_device->_compute_queue, 1, &submit_info, nullptr);
        ASSERT_VULKAN(result);
    }
}

void deferred_renderer::generate_voxel_textures(vk::camera &camera)
{
    voxelize_render_pass::subpass_s& vox_subpass = _voxelize_render_pass.get_subpass(0);
    mrt_render_pass::subpass_s& subpass = _mrt_render_pass.get_subpass(1);
    
    vk::shader_parameter::shader_params_group& voxelize_vertex_params = vox_subpass.get_pipeline(_deferred_image_index).get_uniform_parameters(vk::visual_material::parameter_stage::VERTEX, 0);
    vk::shader_parameter::shader_params_group& voxelize_frag_params = vox_subpass.get_pipeline(_deferred_image_index).get_uniform_parameters(vk::visual_material::parameter_stage::FRAGMENT, 2);
    vk::shader_parameter::shader_params_group& deferred_output_params = subpass.get_pipeline(_deferred_image_index).get_uniform_parameters(vk::visual_material::parameter_stage::FRAGMENT, 5);
    
    voxelize_frag_params["voxel_coords"] = glm::vec3( static_cast<float>(VOXEL_CUBE_WIDTH), static_cast<float>(VOXEL_CUBE_HEIGHT), static_cast<float>(VOXEL_CUBE_DEPTH));
    deferred_output_params["eye_inverse_view_matrix"] = glm::inverse(camera.view_matrix);

    clear_voxels_textures();

    constexpr float distance = 8.f;
    std::array<glm::vec3, 3> cam_positions = {  glm::vec3(0.0f, 0.0f, -distance),glm::vec3(0.0f, distance, 0.0f), glm::vec3(distance, 0.0f, 0.0f)};
    std::array<glm::vec3, 3> up_vectors = { glm::vec3 {0.0f, 1.0f, 0.0f}, glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)};
    
    for( int i = 0; i < _shapes.size(); ++i)
    {
        vox_subpass.get_pipeline(_deferred_image_index).get_dynamic_parameters(vk::visual_material::parameter_stage::VERTEX, 3)[i]["model"] = _shapes[i]->transform.get_transform_matrix();
    }
    
    glm::mat4 project_to_voxel_screen = glm::mat4(1.0f);

    //voxelize, we are going to build the voxel texture from cam_positions.size() views.
    std::array<VkSubmitInfo, 3> submits {};
    
    size_t i = 0;
    std::array<VkPipelineStageFlags, TOTAL_LODS> wait_stage_mask= {};
    std::fill(wait_stage_mask.begin(), wait_stage_mask.end(),VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);
    
    for( ; i < cam_positions.size(); ++i)
    {
        _ortho_camera.position = cam_positions[i];
        _ortho_camera.forward = -_ortho_camera.position;
        
        _ortho_camera.up = up_vectors[i];
        _ortho_camera.update_view_matrix();
        
        if( i == 0)
        {
            //TODO: move this out of this loop
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
        
        vox_subpass.commit_parameters_to_gpu(_deferred_image_index);
        
        submits[i].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submits[i].commandBufferCount = 1;
        
        submits[i].pWaitDstStageMask = wait_stage_mask.data();
        submits[i].waitSemaphoreCount = i == 0 ? 1 : 0;
        submits[i].pWaitSemaphores = i == 0 ? &_generate_voxel_z_axis_semaphore[_deferred_image_index] : nullptr;

        submits[i].pCommandBuffers = &(_voxelize_command_buffers[_deferred_image_index]);
        submits[i].signalSemaphoreCount = 0;
        submits[i].pSignalSemaphores =  nullptr;
        
        //TODO: you need a fence here so that the shader parameters don't get changed while rendering is going on in this loop
        vkResetFences(_device->_logical_device, 1, &_fence);
        vkQueueSubmit(_device->_graphics_queue,1, &submits[i], _fence);
        vkWaitForFences(_device->_logical_device, 1, &_fence, VK_TRUE, UINT64_MAX);
    }

    generate_voxel_mip_maps();
}


void deferred_renderer::draw(camera& camera)
{
    static uint32_t current_frame = 0;
    vkAcquireNextImageKHR(_device->_logical_device, _swapchain->get_vk_swapchain(),
                          std::numeric_limits<uint64_t>::max(),
                          _semaphore_image_available[current_frame], VK_NULL_HANDLE, &_deferred_image_index);
    
    mrt_render_pass::subpass_s& mrt_pass = _mrt_render_pass.get_subpass(0);
    mrt_render_pass::subpass_s& subpass = _mrt_render_pass.get_subpass(1);
    
    vk::shader_parameter::shader_params_group& display_fragment_params = subpass.get_pipeline(_deferred_image_index).
                                            get_uniform_parameters(vk::visual_material::parameter_stage::FRAGMENT, 5) ;
    
    display_fragment_params["world_cam_position"] = glm::vec4(camera.position, 1.0f);
    display_fragment_params["world_light_position"] = _light_pos;
    display_fragment_params["light_color"] = _light_color;
    display_fragment_params["mode"] = static_cast<int>(_rendering_mode);
    
    int binding = 0;
    vk::shader_parameter::shader_params_group& display_params =   subpass.get_pipeline(_deferred_image_index).
                                            get_uniform_parameters(vk::visual_material::parameter_stage::VERTEX, binding);
    
    display_params["width"] = static_cast<float>(_swapchain->get_vk_swap_extent().width);
    display_params["height"] = static_cast<float>(_swapchain->get_vk_swap_extent().height);
    
    perform_final_drawing_setup(_command_buffers[_deferred_image_index], _deferred_image_index );
    generate_voxel_textures(camera);

    mrt_pass.get_pipeline(_deferred_image_index).commit_parameters_to_gpu();
    subpass.get_pipeline(_deferred_image_index).commit_parameters_to_gpu();
    

    std::array<VkSemaphore, 2> wait_semaphores{ _mip_map_semaphores[ _mip_map_semaphores.size()-1][_deferred_image_index],
        _semaphore_image_available[current_frame]};
    //render scene
    VkResult result = {};
    std::array<VkSubmitInfo,1> submit_info = {};
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
    
    result = vkQueueSubmit(_device->_graphics_queue, 1, &submit_info[0], nullptr);

    ASSERT_VULKAN(result);
    //present the scene to viewer
    VkPresentInfoKHR present_info {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pNext = nullptr;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores =&_g_buffers_rendering_done[_deferred_image_index];
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &_swapchain->get_vk_swapchain();
    present_info.pImageIndices = &_deferred_image_index;
    present_info.pResults = nullptr;
    result = vkQueuePresentKHR(_device->_present_queue, &present_info);

    ASSERT_VULKAN(result);
    
    current_frame = (current_frame + 1) % glfw_swapchain::NUM_SWAPCHAIN_IMAGES;

}

void deferred_renderer::destroy()
{
    renderer::destroy();
    _screen_plane.destroy();
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
        vkDestroyFence(_device->_logical_device, _voxel_command_fence[i] , nullptr);
        vkDestroyFence(_device->_logical_device, _fence, nullptr);
        _g_buffers_fence[i] = VK_NULL_HANDLE;
        _voxel_command_fence[i] = VK_NULL_HANDLE;
        _g_buffers_fence[i] = VK_NULL_HANDLE;
        _fence = VK_NULL_HANDLE;
        
        _voxel_2d_view[0][i].destroy();
    }
    
    _mrt_render_pass.destroy();
    _voxelize_render_pass.destroy();
    
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
    
    for( int chain_id = 0; chain_id < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++chain_id)
    {
        for( int texture_id = 0; texture_id < _g_buffer_textures.size(); ++texture_id)
        {
            _g_buffer_textures[chain_id][texture_id].destroy();
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
