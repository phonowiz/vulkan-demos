//
//  deferred_renderer.cpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 4/17/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#include "deferred_renderer.h"
#include "swapchain.h"
#include "texture_2d.h"
#include "../shapes/meshes/mesh.h"

#include <iostream>
#include <assert.h>
using namespace vk;


//based on example code:
//https://github.com/SaschaWillems/Vulkan/blob/master/examples/deferred/deferred.cpp

//great artilce about input attachments and subpasses
//https://www.saschawillems.de/blog/2018/07/19/vulkan-input-attachments-and-sub-passes/

deferred_renderer::deferred_renderer(device* device, GLFWwindow* window, vk::swapchain* swapchain, vk::material_store& store):
renderer(device, window, swapchain, store.GET_MAT<visual_material>("deferred_output")),

_positions(device, swapchain->_swapchain_data.swapchain_extent.width, swapchain->_swapchain_data.swapchain_extent.height, render_texture::usage::COLOR_TARGET),
_albedo(device, swapchain->_swapchain_data.swapchain_extent.width,swapchain->_swapchain_data.swapchain_extent.height, render_texture::usage::COLOR_TARGET),
_normals(device, swapchain->_swapchain_data.swapchain_extent.width, swapchain->_swapchain_data.swapchain_extent.height, render_texture::usage::COLOR_TARGET),
_voxel_2d_view(device, static_cast<float>(VOXEL_CUBE_WIDTH), static_cast<float>(VOXEL_CUBE_HEIGHT), render_texture::usage::COLOR_TARGET),

_depth(device, swapchain->_swapchain_data.swapchain_extent.width, swapchain->_swapchain_data.swapchain_extent.height,true),
_screen_plane(device),
_mrt_material(store.GET_MAT<visual_material>("mrt")),
_mrt_pipeline(device),
_voxelize_pipeline(device, store.GET_MAT<visual_material>("voxelizer")),
_ortho_camera(_voxel_world_dimensions.x, _voxel_world_dimensions.y, _voxel_world_dimensions.z)
{
    int binding = 0;
    
    for( size_t i = 0; i < _voxel_albedo_textures.size(); ++i)
    {
        _clear_voxel_texture_pipeline[i].set_device(_device);
        _clear_voxel_texture_pipeline[i].set_material(store.GET_MAT<compute_material>("clear_3d_texture"));
        
        _voxel_albedo_textures[i].set_device(device);
        _voxel_albedo_textures[i].set_dimensions(VOXEL_CUBE_WIDTH >> i,  VOXEL_CUBE_HEIGHT >> i, VOXEL_CUBE_DEPTH >> i );
        
        _voxel_normal_textures[i].set_device(device);
        _voxel_normal_textures[i].set_dimensions(VOXEL_CUBE_WIDTH >> i,  VOXEL_CUBE_HEIGHT >> i, VOXEL_CUBE_DEPTH >> i );
        
        _voxel_albedo_textures[i].set_filter(image::filter::LINEAR);
        _voxel_normal_textures[i].set_filter(image::filter::LINEAR);
        
        _voxel_normal_textures[i].set_image_layout(image::image_layouts::SHADER_READ_ONLY_OPTIMAL);
        _voxel_albedo_textures[i].set_image_layout(image::image_layouts::SHADER_READ_ONLY_OPTIMAL);

        
        assert(_voxel_albedo_textures[i].get_height() % compute_pipeline::LOCAL_GROUP_SIZE == 0 && "voxel texture will not clear properly with these dimensions");
        assert(_voxel_albedo_textures[i].get_width() % compute_pipeline::LOCAL_GROUP_SIZE == 0 && "voxel texture will not clear properly with these dimensions");
        assert(_voxel_albedo_textures[i].get_depth() % compute_pipeline::LOCAL_GROUP_SIZE == 0 && "voxel texture will not clear properly with these dimensions");
    }
    
    _mrt_material->init_parameter("view", visual_material::parameter_stage::VERTEX, glm::mat4(0), binding);
    _mrt_material->init_parameter("projection", visual_material::parameter_stage::VERTEX, glm::mat4(0), binding);
    _mrt_material->init_parameter("lightPosition", visual_material::parameter_stage::VERTEX, glm::vec3(0), binding);
    
    _pipeline._material->init_parameter("width", visual_material::parameter_stage::VERTEX, 0.f, 0);
    _pipeline._material->init_parameter("height", visual_material::parameter_stage::VERTEX, 0.f, 0);
    

    
    _voxelize_pipeline.set_image_sampler(&_voxel_albedo_textures[0], "voxel_albedo_texture",
                                         visual_material::parameter_stage::FRAGMENT, 1, resource::usage_type::STORAGE_IMAGE);
    _voxelize_pipeline.set_image_sampler(&_voxel_normal_textures[0], "voxel_normal_texture",
                                         visual_material::parameter_stage::FRAGMENT, 4, resource::usage_type::STORAGE_IMAGE);
    
    _voxelize_pipeline._material->init_parameter("inverse_view_projection", visual_material::parameter_stage::FRAGMENT, glm::mat4(1.0f), 2);
    _voxelize_pipeline._material->init_parameter("project_to_voxel_screen", visual_material::parameter_stage::FRAGMENT, glm::mat4(1.0f), 2);
    _voxelize_pipeline._material->init_parameter("voxel_coords", visual_material::parameter_stage::FRAGMENT, glm::vec3(1.0f), 2);
    
    _voxelize_pipeline._material->init_parameter("view", visual_material::parameter_stage::VERTEX, glm::mat4(1.0f), 0);
    _voxelize_pipeline._material->init_parameter("projection", visual_material::parameter_stage::VERTEX, glm::mat4(1.0f), 0);
    _voxelize_pipeline._material->init_parameter("light_position", visual_material::parameter_stage::VERTEX, glm::vec3(1.0f), 0);
    _voxelize_pipeline._material->init_parameter("eye_position", visual_material::parameter_stage::VERTEX, glm::vec3(1.0f), 0);
    
    setup_sampling_rays();
    
    glm::vec4 world_scale_voxel = glm::vec4(float(_voxel_world_dimensions.x/VOXEL_CUBE_WIDTH),
                                            float(_voxel_world_dimensions.y/VOXEL_CUBE_HEIGHT),
                                            float(_voxel_world_dimensions.z/VOXEL_CUBE_DEPTH), 1.0f);
    
    _pipeline._material->init_parameter("world_cam_position", visual_material::parameter_stage::FRAGMENT, glm::vec4(0.0f), 5);
    _pipeline._material->init_parameter("world_light_position", visual_material::parameter_stage::FRAGMENT, glm::vec3(0.0f), 5);
    _pipeline._material->init_parameter("light_color", visual_material::parameter_stage::FRAGMENT, glm::vec4(0.0f), 5);
    _pipeline._material->init_parameter("voxel_size_in_world_space", visual_material::parameter_stage::FRAGMENT, world_scale_voxel, 5);
    _pipeline._material->init_parameter("state", visual_material::parameter_stage::FRAGMENT, int(0), 5);
    _pipeline._material->init_parameter("sampling_rays", visual_material::parameter_stage::FRAGMENT, _sampling_rays.data(), _sampling_rays.size(), 5);
    _pipeline._material->init_parameter("vox_view_projection", visual_material::parameter_stage::FRAGMENT, glm::mat4(1.0f), 5);
    _pipeline._material->init_parameter("1", visual_material::parameter_stage::FRAGMENT, int(4), 5);
    _pipeline._material->init_parameter("eye_in_world_space", visual_material::parameter_stage::FRAGMENT, glm::vec3(0), 5);
    
    _pipeline._material->set_image_sampler(&_voxel_normal_textures[0], "voxel_normals", visual_material::parameter_stage::FRAGMENT, 6, material_base::usage_type::COMBINED_IMAGE_SAMPLER);
    _pipeline._material->set_image_sampler(&_voxel_albedo_textures[0], "voxel_albedos", visual_material::parameter_stage::FRAGMENT, 7, material_base::usage_type::COMBINED_IMAGE_SAMPLER);
    
    _pipeline._material->set_image_sampler(&_voxel_albedo_textures[1], "voxel_albedos1", visual_material::parameter_stage::FRAGMENT, 8, material_base::usage_type::COMBINED_IMAGE_SAMPLER);
    _pipeline._material->set_image_sampler(&_voxel_albedo_textures[2], "voxel_albedos2", visual_material::parameter_stage::FRAGMENT, 9, material_base::usage_type::COMBINED_IMAGE_SAMPLER);
    _pipeline._material->set_image_sampler(&_voxel_albedo_textures[3], "voxel_albedos3", visual_material::parameter_stage::FRAGMENT, 10, material_base::usage_type::COMBINED_IMAGE_SAMPLER);
    _pipeline._material->set_image_sampler(&_voxel_albedo_textures[3], "voxel_albedos4", visual_material::parameter_stage::FRAGMENT, 14, material_base::usage_type::COMBINED_IMAGE_SAMPLER);
    
    _pipeline._material->set_image_sampler(&_voxel_normal_textures[1], "voxel_normals1", visual_material::parameter_stage::FRAGMENT, 11, material_base::usage_type::COMBINED_IMAGE_SAMPLER);
    _pipeline._material->set_image_sampler(&_voxel_normal_textures[2], "voxel_normals2", visual_material::parameter_stage::FRAGMENT, 12, material_base::usage_type::COMBINED_IMAGE_SAMPLER);
    _pipeline._material->set_image_sampler(&_voxel_normal_textures[3], "voxel_normals3", visual_material::parameter_stage::FRAGMENT, 13, material_base::usage_type::COMBINED_IMAGE_SAMPLER);
    _pipeline._material->set_image_sampler(&_voxel_normal_textures[3], "voxel_normals4", visual_material::parameter_stage::FRAGMENT, 15, material_base::usage_type::COMBINED_IMAGE_SAMPLER);
    //_material->print_uniform_argument_names();
    
    _positions.set_filter(image::filter::NEAREST);
    _albedo.set_filter(image::filter::NEAREST);
    _normals.set_filter(image::filter::NEAREST);
    _depth.set_filter(image::filter::NEAREST);
    
    _positions.init();
    _albedo.init();
    _normals.init();
    _depth.init();
    
    for( size_t i = 0; i < _voxel_normal_textures.size(); ++i)
    {
        _voxel_albedo_textures[i].init();
        _voxel_normal_textures[i].init();
    }
    
    _voxel_2d_view.set_filter(image::filter::NEAREST);
    _voxel_2d_view.init();
    
    _voxelize_pipeline.set_cullmode( graphics_pipeline::cull_mode::NONE);
    _voxelize_pipeline.set_depth_enable(false);
    
    _mrt_pipeline.set_material(_mrt_material);
    _screen_plane.create();
    
    create_command_buffers(&_offscreen_command_buffers, _device->_graphics_command_pool);
    create_command_buffers(&_voxelize_command_buffers, _device->_graphics_command_pool);
    
    for( int i = 0; i < _clear_3d_texture_command_buffers.size(); ++i)
    {
        create_command_buffers(&_clear_3d_texture_command_buffers[i], _device->_compute_command_pool);
        _create_voxel_mip_maps_pipelines[i].set_device(_device);
        _create_voxel_mip_maps_pipelines[i].set_material(store.GET_MAT<compute_material>("downsize"));
    }
    
    for(int i = 0; i < _genered_3d_mip_maps_commands.size(); ++i)
    {
        create_command_buffers(&_genered_3d_mip_maps_commands[i], _device->_compute_command_pool);
    }

}

void deferred_renderer::create_voxel_texture_pipelines()
{
    //TODO: you'll need to clear normals as well
    for( int i = 0; i < _clear_voxel_texture_pipeline.size(); ++i)
    {
        _clear_voxel_texture_pipeline[i].material->set_image_sampler(&_voxel_albedo_textures[i],
                                                               "texture_3d", material_base::parameter_stage::COMPUTE, 0, material_base::usage_type::STORAGE_IMAGE);
        _clear_voxel_texture_pipeline[i].material->commit_parameters_to_gpu();
    }
    
    for(int i =0; i < _create_voxel_mip_maps_pipelines.size(); ++i)
    {
        _create_voxel_mip_maps_pipelines[i].material->set_image_sampler(&_voxel_albedo_textures[i], "r_texture_1", material_base::parameter_stage::COMPUTE,
                                                        0, material_base::usage_type::COMBINED_IMAGE_SAMPLER);
        _create_voxel_mip_maps_pipelines[i].material->set_image_sampler(&_voxel_normal_textures[i], "r_texture_2", material_base::parameter_stage::COMPUTE,
                                                        1, material_base::usage_type::COMBINED_IMAGE_SAMPLER);
        _create_voxel_mip_maps_pipelines[i].material->set_image_sampler(&_voxel_albedo_textures[i + 1], "w_texture_1", material_base::parameter_stage::COMPUTE,
                                                        2, material_base::usage_type::STORAGE_IMAGE);
        _create_voxel_mip_maps_pipelines[i].material->set_image_sampler(&_voxel_normal_textures[i + 1], "w_texture_2", material_base::parameter_stage::COMPUTE,
                                                        3, material_base::usage_type::STORAGE_IMAGE);
        
        _create_voxel_mip_maps_pipelines[i].material->commit_parameters_to_gpu();
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

void deferred_renderer::create_voxelization_render_pass()
{
    
    constexpr uint32_t NUMBER_OUTPUT_ATTACHMENTS = 1;
    std::array<VkAttachmentDescription, NUMBER_OUTPUT_ATTACHMENTS> attachment_descriptions {};
    
    for( uint32_t i = 0; i < attachment_descriptions.size(); ++i)
    {
        attachment_descriptions[i].samples = VK_SAMPLE_COUNT_1_BIT;
        attachment_descriptions[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment_descriptions[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment_descriptions[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment_descriptions[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        
        attachment_descriptions[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachment_descriptions[i].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
    
    attachment_descriptions[0].format = static_cast<VkFormat>(_voxel_2d_view.get_format());
    
    std::array<VkAttachmentReference, NUMBER_OUTPUT_ATTACHMENTS> color_references {};
    //note: the first integer in the instruction is the attachment location specified in the shader
    
    //todo: eventually this color attachment will not be needed, please remove, for now, it is here for debugging purposes
    color_references[0] = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.pColorAttachments = color_references.data();
    subpass.colorAttachmentCount = static_cast<uint32_t>(color_references.size());
    subpass.pDepthStencilAttachment = nullptr;
    
    
    // Use subpass dependencies for attachment layput transitions
    std::array<VkSubpassDependency, 2> dependencies {};
    
    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    
    
    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    
    
    VkRenderPassCreateInfo render_pass_info = {};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.pAttachments = attachment_descriptions.data();
    render_pass_info.attachmentCount = attachment_descriptions.size();
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = 2;
    render_pass_info.pDependencies = dependencies.data();
    
    VkResult result = vkCreateRenderPass(_device->_logical_device, &render_pass_info, nullptr, &_voxelization_render_pass);
    ASSERT_VULKAN(result);
}

void deferred_renderer::create_render_pass()
{
    //renderer::create_render_pass call will create the swap chain render pass, or the render pass that will display stuff
    //to screen
    renderer::create_render_pass();
    
    //note: for an exellent explanation of attachments, go here:
    //https://stackoverflow.com/questions/46384007/vulkan-what-is-the-meaning-of-attachment
    
    constexpr uint32_t NUMBER_OUTPUT_ATTACHMENTS = 4;
    std::array<VkAttachmentDescription, NUMBER_OUTPUT_ATTACHMENTS> attachment_descriptions {};
    
    for( uint32_t i = 0; i <= attachment_descriptions.size() - 2; ++i)
    {
        attachment_descriptions[i].samples = VK_SAMPLE_COUNT_1_BIT;
        attachment_descriptions[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment_descriptions[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment_descriptions[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment_descriptions[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        
        attachment_descriptions[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachment_descriptions[i].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
    
    //note: the last attachment is the depth texture, check create_frame_buffers function, frame buffer will assume this as well
    attachment_descriptions[attachment_descriptions.size() - 1] = _depth.get_depth_attachment();
    
    attachment_descriptions[0].format = static_cast<VkFormat>(_normals.get_format());
    attachment_descriptions[1].format = static_cast<VkFormat>(_albedo.get_format());
    attachment_descriptions[2].format = static_cast<VkFormat>(_positions.get_format());
    attachment_descriptions[3].format = static_cast<VkFormat>(_depth.get_format());
    
    std::array<VkAttachmentReference, 3> color_references {};
    //note: the first integer in the instruction is the attachment location specified in the shader
    color_references[0] = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    color_references[1] = { 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    color_references[2] = { 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    //color_references[3] = { 3, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
    
    VkAttachmentReference depth_reference = {};
    //note: in this code, the last attachement is the depth
    depth_reference.attachment = attachment_descriptions.size() -1;
    depth_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.pColorAttachments = color_references.data();
    subpass.colorAttachmentCount = static_cast<uint32_t>(color_references.size());
    subpass.pDepthStencilAttachment = &depth_reference;
    
    
    // Use subpass dependencies for attachment layput transitions
    std::array<VkSubpassDependency, 2> dependencies {};
    
    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    
    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    
    
    VkRenderPassCreateInfo render_pass_info = {};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.pAttachments = attachment_descriptions.data();
    render_pass_info.attachmentCount = static_cast<uint32_t>(attachment_descriptions.size());
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = static_cast<uint32_t>(dependencies.size());
    render_pass_info.pDependencies = dependencies.data();
    
    vkCreateRenderPass(_device->_logical_device, &render_pass_info, nullptr, &_mrt_render_pass);
    
    create_voxelization_render_pass();
}

void deferred_renderer::create_semaphores_and_fences()
{
    renderer::create_semaphores_and_fences();
    
    create_semaphore(_deferred_semaphore_image_available);
    create_semaphore(_g_buffers_rendering_done);
    create_semaphore(_voxelize_semaphore);
    create_semaphore(_voxelize_semaphore_done);
    
    create_semaphore(_generate_voxel_z_axis_semaphore);
    create_semaphore(_generate_voxel_y_axis_semaphore);
    create_semaphore(_generate_voxel_x_axis_semaphore);
    
    create_semaphore(_clear_voxel_textures);
    
    for( size_t i = 0; i < _mip_map_semaphores.size(); ++i)
    {
        create_semaphore(_mip_map_semaphores[i]);
    }
    
    assert(NUM_OF_FRAMES == _swapchain->_swapchain_data.image_set.get_image_count());
    for(int i = 0; i < _swapchain->_swapchain_data.image_set.get_image_count(); ++i)
    {
        create_fence(_g_buffers_fence[i]);
        create_fence(_voxelize_inflight_fence[i]);
        create_fence(_voxel_command_fence[i]);
    }
}

void deferred_renderer::create_pipeline()
{
    _pipeline.create(_render_pass,
                     _swapchain->_swapchain_data.swapchain_extent.width,
                     _swapchain->_swapchain_data.swapchain_extent.height);
    
    _mrt_pipeline.set_number_of_blend_attachments(3);
    bool enable_blend = true;
    _mrt_pipeline.modify_attachment_blend(0, graphics_pipeline::write_channels::RGBA, enable_blend);
    _mrt_pipeline.modify_attachment_blend(1, graphics_pipeline::write_channels::RGBA, false);
    _mrt_pipeline.modify_attachment_blend(2, graphics_pipeline::write_channels::RGBA, false);
    
    _mrt_pipeline.create(_mrt_render_pass,
                         _swapchain->_swapchain_data.swapchain_extent.width,
                         _swapchain->_swapchain_data.swapchain_extent.height);
    
    _voxelize_pipeline.set_number_of_blend_attachments(1);
    _voxelize_pipeline.modify_attachment_blend(0, graphics_pipeline::write_channels::RGBA, false);
    _voxelize_pipeline.create(_voxelization_render_pass, VOXEL_CUBE_WIDTH, VOXEL_CUBE_HEIGHT);
    
    create_voxel_texture_pipelines();
}


void deferred_renderer::create_frame_buffers()
{
    renderer::create_frame_buffers();
    
    //TODO: Get rid of the vector class
    _deferred_swapchain_frame_buffers.resize(_swapchain->_swapchain_data.image_set.get_image_count());
    _voxelize_frame_buffers.resize(_swapchain->_swapchain_data.image_set.get_image_count());
    
    for (size_t i = 0; i < _swapchain_frame_buffers.size(); i++)
    {
        std::array<VkImageView, 4> attachment_views {};
        
        attachment_views[0] = _normals._image_view;
        attachment_views[1] = _albedo._image_view;
        attachment_views[2] = _positions._image_view;
        //note: the create_render_pass function specifies that the last attachment is the depth
        attachment_views[3] = _depth._image_view;
        
        VkFramebufferCreateInfo framebuffer_create_info {};
        framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_create_info.pNext = nullptr;
        framebuffer_create_info.flags = 0;
        framebuffer_create_info.renderPass = _mrt_render_pass;
        framebuffer_create_info.attachmentCount = attachment_views.size();
        framebuffer_create_info.pAttachments = attachment_views.data();
        framebuffer_create_info.width = _swapchain->_swapchain_data.swapchain_extent.width;
        framebuffer_create_info.height = _swapchain->_swapchain_data.swapchain_extent.height;
        framebuffer_create_info.layers = 1;
        
        VkResult result = vkCreateFramebuffer(_device->_logical_device, &framebuffer_create_info, nullptr, &(_deferred_swapchain_frame_buffers[i]));
        ASSERT_VULKAN(result)
        
        //voxelization frame buffer
        std::array<VkImageView, 1> voxel_attachment_views {};
        
        voxel_attachment_views[0] = _voxel_2d_view._image_view;
        
        framebuffer_create_info = {};
        framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_create_info.pNext = nullptr;
        framebuffer_create_info.flags = 0;
        framebuffer_create_info.renderPass = _voxelization_render_pass;
        framebuffer_create_info.attachmentCount = voxel_attachment_views.size();
        framebuffer_create_info.pAttachments = voxel_attachment_views.data();
        framebuffer_create_info.width = VOXEL_CUBE_WIDTH;
        framebuffer_create_info.height = VOXEL_CUBE_HEIGHT;
        framebuffer_create_info.layers = 1;
        
        result = vkCreateFramebuffer(_device->_logical_device, &framebuffer_create_info, nullptr, &(_voxelize_frame_buffers[i]));
        
        ASSERT_VULKAN(result);
    }
}


void deferred_renderer::record_voxelize_command_buffers(obj_shape** shapes, size_t number_of_meshes)
{
    assert(_voxelize_pipeline._pipeline != VK_NULL_HANDLE);
    assert(_voxelize_pipeline._pipeline_layout != VK_NULL_HANDLE);
    
    std::array<VkClearValue,1> clear_values;
    clear_values[0].color = { { 1.0f, 1.0f, 1.0f, 0.0f } };
    
    for( int i = 0; i < _swapchain->_swapchain_data.image_set.get_image_count(); ++i)
    {
        VkRenderPassBeginInfo render_pass_begin_info {};
        render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_begin_info.pNext = nullptr;
        render_pass_begin_info.renderPass = _voxelization_render_pass;
        render_pass_begin_info.framebuffer = _voxelize_frame_buffers[i];
        render_pass_begin_info.renderArea.offset = { 0, 0 };
        render_pass_begin_info.renderArea.extent = { VOXEL_CUBE_WIDTH, VOXEL_CUBE_HEIGHT };
        
        render_pass_begin_info.clearValueCount = clear_values.size();
        render_pass_begin_info.pClearValues = clear_values.data();
        
        VkCommandBufferBeginInfo command_buffer_begin_info {};
        command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_begin_info.pNext = nullptr;
        command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        command_buffer_begin_info.pInheritanceInfo = nullptr;
        
        VkResult result = vkBeginCommandBuffer(_voxelize_command_buffers[i], &command_buffer_begin_info);
        ASSERT_VULKAN(result);
        
        // Image memory barrier to make sure that compute shader writes are finished before sampling from the texture
        std::array<VkImageMemoryBarrier, 2> image_memory_barrier = {};
        image_memory_barrier[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        
        constexpr uint32_t VK_FLAGS_NONE = 0;

        
        vkCmdBeginRenderPass(_voxelize_command_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(_voxelize_command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, _voxelize_pipeline._pipeline);
        
        // We won't be changing the layout of the image
        image_memory_barrier[0].oldLayout = static_cast<VkImageLayout>(_voxel_albedo_textures[0].get_image_layout());
        image_memory_barrier[0].newLayout = static_cast<VkImageLayout>(_voxel_albedo_textures[0].get_image_layout());
        image_memory_barrier[0].image = _voxel_albedo_textures[0].get_image();
        image_memory_barrier[0].subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        image_memory_barrier[0].srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
        image_memory_barrier[0].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        image_memory_barrier[0].srcQueueFamilyIndex = _device->_indices.graphics_family.value();
        image_memory_barrier[0].dstQueueFamilyIndex = _device->_indices.graphics_family.value();
        
        image_memory_barrier[1] = image_memory_barrier[0];
        image_memory_barrier[1].image = _voxel_normal_textures[0].get_image();
        image_memory_barrier[1].oldLayout = static_cast<VkImageLayout>(_voxel_normal_textures[0].get_image_layout());
        image_memory_barrier[1].newLayout = static_cast<VkImageLayout>(_voxel_normal_textures[0].get_image_layout());
        vkCmdPipelineBarrier(
                             _voxelize_command_buffers[i],
                             VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                             VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                             VK_FLAGS_NONE,
                             0, nullptr,
                             0, nullptr,
                             image_memory_barrier.size(), image_memory_barrier.data());
        
        VkViewport viewport {};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = VOXEL_CUBE_WIDTH;
        viewport.height = VOXEL_CUBE_HEIGHT;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(_voxelize_command_buffers[i], 0, 1, &viewport);
        
        VkRect2D scissor {};
        scissor.offset = { 0, 0};
        scissor.extent = { VOXEL_CUBE_WIDTH, VOXEL_CUBE_HEIGHT};
        vkCmdSetScissor(_voxelize_command_buffers[i], 0, 1, &scissor);
        
        for( uint32_t j = 0; j < number_of_meshes; ++j)
        {
            shapes[j]->draw(_voxelize_command_buffers[i], _voxelize_pipeline, j);
        }
        
        vkCmdEndRenderPass(_voxelize_command_buffers[i]);
        vkEndCommandBuffer(_voxelize_command_buffers[i]);
    }
}

void deferred_renderer::record_3d_mip_maps_commands()
{
    VkCommandBufferBeginInfo command_buffer_begin_info {};
    command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_begin_info.pInheritanceInfo = nullptr;
    
    assert(NUM_OF_FRAMES ==_swapchain->_swapchain_data.image_set.get_image_count());
    for( int i = 0; i < _genered_3d_mip_maps_commands.size(); ++i)
    {
        for( int j = 0; j < _swapchain->_swapchain_data.image_set.get_image_count(); ++j)
        {
            assert((VOXEL_CUBE_WIDTH >> i) % compute_pipeline::LOCAL_GROUP_SIZE == 0 && "invalid voxel cube size, voxel texture will not clear properly");
            assert((VOXEL_CUBE_HEIGHT >> i) % compute_pipeline::LOCAL_GROUP_SIZE == 0 && "invalid voxel cube size, voxel texture will not clear properly");
            assert((VOXEL_CUBE_DEPTH >> i) % compute_pipeline::LOCAL_GROUP_SIZE == 0 && "invalid voxel cube size, voxel texture will not clear properly");

            uint32_t local_groups_x = (VOXEL_CUBE_WIDTH >> i) / compute_pipeline::LOCAL_GROUP_SIZE;
            uint32_t local_groups_y = (VOXEL_CUBE_HEIGHT >> i) / compute_pipeline::LOCAL_GROUP_SIZE;
            uint32_t local_groups_z = (VOXEL_CUBE_DEPTH >> i) / compute_pipeline::LOCAL_GROUP_SIZE;

            if( i != 0)
            {
                 _create_voxel_mip_maps_pipelines[i].record_begin_commands( [=]()
                 {
                     std::array<VkImageMemoryBarrier, 2> image_memory_barrier {};
                     image_memory_barrier[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                     image_memory_barrier[0].pNext = nullptr;
                     image_memory_barrier[0].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
                     image_memory_barrier[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                     image_memory_barrier[0].oldLayout = static_cast<VkImageLayout>(_voxel_albedo_textures[i-1].get_image_layout());;
                     image_memory_barrier[0].newLayout = static_cast<VkImageLayout>(_voxel_albedo_textures[i-1].get_image_layout());
                     image_memory_barrier[0].image = _voxel_albedo_textures[i-1].get_image();
                     image_memory_barrier[0].subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
                     image_memory_barrier[0].srcQueueFamilyIndex = _device->_indices.graphics_family.value();
                     image_memory_barrier[0].dstQueueFamilyIndex = _device->_indices.graphics_family.value();

                     image_memory_barrier[1] = image_memory_barrier[0];
                     image_memory_barrier[1].image = _voxel_normal_textures[i-1].get_image();
                     image_memory_barrier[1].oldLayout = static_cast<VkImageLayout>(_voxel_normal_textures[i-1].get_image_layout());;
                     image_memory_barrier[1].newLayout = static_cast<VkImageLayout>(_voxel_normal_textures[i-1].get_image_layout());
                     constexpr uint32_t VK_FLAGS_NONE = 0;

                     vkCmdPipelineBarrier(
                                          _genered_3d_mip_maps_commands[i][j],
                                          VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                          VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                          VK_FLAGS_NONE,
                                          0, nullptr,
                                          0, nullptr,
                                          image_memory_barrier.size(), image_memory_barrier.data());
                 } );
            }


            _create_voxel_mip_maps_pipelines[i].record_dispatch_commands(_genered_3d_mip_maps_commands[i][j],
                                                                local_groups_x, local_groups_y, local_groups_z);

        }
    }
}

void deferred_renderer::record_clear_texture_3d_buffer()
{
    VkCommandBufferBeginInfo command_buffer_begin_info {};
    command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_begin_info.pInheritanceInfo = nullptr;
    
    assert(NUM_OF_FRAMES ==_swapchain->_swapchain_data.image_set.get_image_count());
    for( int i = 0; i < _clear_3d_texture_command_buffers.size(); ++i)
    {
        for( int j = 0; j < _swapchain->_swapchain_data.image_set.get_image_count(); ++j)
        {
            assert((VOXEL_CUBE_WIDTH >> i) % compute_pipeline::LOCAL_GROUP_SIZE == 0 && "invalid voxel cube size, voxel texture will not clear properly");
            assert((VOXEL_CUBE_HEIGHT >> i) % compute_pipeline::LOCAL_GROUP_SIZE == 0 && "invalid voxel cube size, voxel texture will not clear properly");
            assert((VOXEL_CUBE_DEPTH >> i) % compute_pipeline::LOCAL_GROUP_SIZE == 0 && "invalid voxel cube size, voxel texture will not clear properly");
            
            uint32_t local_groups_x = (VOXEL_CUBE_WIDTH >> i) / compute_pipeline::LOCAL_GROUP_SIZE;
            uint32_t local_groups_y = (VOXEL_CUBE_HEIGHT >> i) / compute_pipeline::LOCAL_GROUP_SIZE;
            uint32_t local_groups_z = (VOXEL_CUBE_DEPTH >> i) / compute_pipeline::LOCAL_GROUP_SIZE;
            
            _clear_voxel_texture_pipeline[i].record_dispatch_commands(_clear_3d_texture_command_buffers[i][j],
                                                                local_groups_x, local_groups_y, local_groups_z);
        }
    }
}

void deferred_renderer::record_command_buffers(obj_shape** shapes, size_t number_of_shapes)
{
    VkCommandBufferBeginInfo command_buffer_begin_info {};
    command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_begin_info.pNext = nullptr;
    command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    command_buffer_begin_info.pInheritanceInfo = nullptr;
    
    // Clear values for all attachments written in the fragment sahder
    std::array<VkClearValue,4> clear_values;
    clear_values[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
    clear_values[1].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
    clear_values[2].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
    clear_values[3].depthStencil = { 1.0f, 0 };
    
    for (size_t i = 0; i < _swapchain->_swapchain_data.image_set.get_image_count(); i++)
    {
        //TODO: consider creating a render pass object which contains a frame buffer object and all of the assets needed to
        //render a mesh.
        
        VkRenderPassBeginInfo render_pass_begin_info {};
        render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_begin_info.pNext = nullptr;
        render_pass_begin_info.renderPass = _mrt_render_pass;
        render_pass_begin_info.framebuffer = _deferred_swapchain_frame_buffers[i];
        render_pass_begin_info.renderArea.offset = { 0, 0 };
        render_pass_begin_info.renderArea.extent = { _swapchain->_swapchain_data.swapchain_extent.width,
            _swapchain->_swapchain_data.swapchain_extent.height };
        
        render_pass_begin_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
        render_pass_begin_info.pClearValues = clear_values.data();
        
        VkCommandBufferBeginInfo command_buffer_begin_info {};
        command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_begin_info.pNext = nullptr;
        command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        command_buffer_begin_info.pInheritanceInfo = nullptr;
        
        VkResult result = vkBeginCommandBuffer(_offscreen_command_buffers[i], &command_buffer_begin_info);
        ASSERT_VULKAN(result);
        
        vkCmdBeginRenderPass(_offscreen_command_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(_offscreen_command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, _mrt_pipeline._pipeline);
        
        VkViewport viewport {};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = _swapchain->_swapchain_data.swapchain_extent.width;
        viewport.height = _swapchain->_swapchain_data.swapchain_extent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(_offscreen_command_buffers[i], 0, 1, &viewport);
        
        VkRect2D scissor {};
        scissor.offset = { 0, 0};
        scissor.extent = { _swapchain->_swapchain_data.swapchain_extent.width,
            _swapchain->_swapchain_data.swapchain_extent.height};
        vkCmdSetScissor(_offscreen_command_buffers[i], 0, 1, &scissor);
        
        for( uint32_t j = 0; j < number_of_shapes; ++j)
        {
            shapes[j]->draw(_offscreen_command_buffers[i], _mrt_pipeline, j);
        }
        
        vkCmdEndRenderPass(_offscreen_command_buffers[i]);
        vkEndCommandBuffer(_offscreen_command_buffers[i]);
    }
    
    record_voxelize_command_buffers(shapes, number_of_shapes);
    record_clear_texture_3d_buffer();
    record_3d_mip_maps_commands();
    
    std::array<obj_shape*, 1> screen_plane_array = { &_screen_plane };
    renderer::record_command_buffers(screen_plane_array.data(), screen_plane_array.size());
}

void deferred_renderer::destroy_framebuffers()
{
    renderer::destroy_framebuffers();
    
    for (size_t i = 0; i < _swapchain_frame_buffers.size(); i++)
    {
        vkDestroyFramebuffer(_device->_logical_device, _deferred_swapchain_frame_buffers[i], nullptr);
        _deferred_swapchain_frame_buffers[i] = VK_NULL_HANDLE;
        vkDestroyFramebuffer(_device->_logical_device, _voxelize_frame_buffers[i], nullptr);
        _voxelize_frame_buffers[i] = VK_NULL_HANDLE;
    }
}

void deferred_renderer::perform_final_drawing_setup()
{
    if( !_setup_initialized)
    {
        _pipeline.set_image_sampler(&_normals, "normals", visual_material::parameter_stage::FRAGMENT, 1, resource::usage_type::COMBINED_IMAGE_SAMPLER);
        _pipeline.set_image_sampler(&_albedo, "albedo", visual_material::parameter_stage::FRAGMENT, 2, resource::usage_type::COMBINED_IMAGE_SAMPLER);
        _pipeline.set_image_sampler(&_positions, "world_positions", visual_material::parameter_stage::FRAGMENT, 3, resource::usage_type::COMBINED_IMAGE_SAMPLER);
        _pipeline.set_image_sampler(&_depth, "depth", visual_material::parameter_stage::FRAGMENT, 4, resource::usage_type::COMBINED_IMAGE_SAMPLER);
        _pipeline.set_image_sampler(&_voxel_normal_textures[0], "voxel_normals", visual_material::parameter_stage::FRAGMENT, 6, material_base::usage_type::COMBINED_IMAGE_SAMPLER);
        _pipeline.set_image_sampler(&_voxel_albedo_textures[0], "voxel_albedos", visual_material::parameter_stage::FRAGMENT, 7, material_base::usage_type::COMBINED_IMAGE_SAMPLER);
        _setup_initialized = true;
    }
    
    for( int i = 0; i < _shapes.size(); ++i)
    {
        _voxelize_pipeline._material->get_dynamic_parameters(vk::visual_material::parameter_stage::VERTEX, 3)[i]["model"] = _shapes[i]->transform.get_transform_matrix();
    }
    
    _voxelize_pipeline._material->commit_parameters_to_gpu();
    _mrt_pipeline._material->commit_parameters_to_gpu();
    
    renderer::perform_final_drawing_setup();
}

void deferred_renderer::clear_voxels_textures()
{
//    std::array<VkCommandBuffer*, 4> clear_command_buffers = {_clear_3d_texture_command_buffers_1, _clear_3d_texture_command_buffers_2, _clear_3d_texture_command_buffers_3,
//        _clear_3d_texture_command_buffers_4};
    
    std::array<VkCommandBuffer, 4> clear_commands {};
    
    for( int i =0; i < _clear_3d_texture_command_buffers.size(); ++i)
    {
        clear_commands[i] = _clear_3d_texture_command_buffers[i][_deferred_image_index];
    }
    //The clearing of voxels happens in parallel
    VkSubmitInfo submit_info = {};
    submit_info.commandBufferCount = clear_commands.size();
    submit_info.pCommandBuffers = clear_commands.data();
    submit_info.waitSemaphoreCount = 0;
    submit_info.pWaitSemaphores = nullptr;
    submit_info.pSignalSemaphores = &_clear_voxel_textures;
    submit_info.signalSemaphoreCount = 1;
    
    VkResult result = vkQueueSubmit(_device->_compute_queue, 1, &submit_info, _voxel_command_fence[_deferred_image_index]);
    
    vkResetFences(_device->_logical_device, 1, &_voxel_command_fence[_deferred_image_index]);
    vkWaitForFences(_device->_logical_device, 1, &_voxel_command_fence[_deferred_image_index], VK_TRUE, std::numeric_limits<uint64_t>::max());
    ASSERT_VULKAN(result);
}

void deferred_renderer::generate_voxel_mip_maps()
{
    //TODO: batch these into one submit...
    for( int i = 0; i < _genered_3d_mip_maps_commands.size(); ++i)
    {
        VkSubmitInfo submit_info = {};
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &_genered_3d_mip_maps_commands[i][_deferred_image_index];
        submit_info.waitSemaphoreCount =  0;//1;
        submit_info.pWaitSemaphores = nullptr;
        submit_info.pSignalSemaphores = nullptr;//&_mip_map_semaphores[i];
        submit_info.signalSemaphoreCount = 0;//1;
        
        VkResult result = vkQueueSubmit(_device->_compute_queue, 1, &submit_info,
                                        i == _genered_3d_mip_maps_commands.size() -1 ? _voxel_command_fence[0] : nullptr);
        ASSERT_VULKAN(result);
    }
    
    vkResetFences(_device->_logical_device, 1, &_voxel_command_fence[0]);
    vkWaitForFences(_device->_logical_device, 1, &_voxel_command_fence[0], VK_TRUE, std::numeric_limits<uint64_t>::max());
}

void deferred_renderer::generate_voxel_textures(vk::camera &camera)
{
    vk::shader_parameter::shader_params_group& voxelize_vertex_params = _voxelize_pipeline._material->get_uniform_parameters(vk::visual_material::parameter_stage::VERTEX, 0);
    vk::shader_parameter::shader_params_group& voxelize_frag_params = _voxelize_pipeline._material->get_uniform_parameters(vk::visual_material::parameter_stage::FRAGMENT, 2);
    vk::shader_parameter::shader_params_group& deferred_output_params = _pipeline._material->get_uniform_parameters(vk::visual_material::parameter_stage::FRAGMENT, 5);
    
    voxelize_frag_params["voxel_coords"] = glm::vec3( static_cast<float>(VOXEL_CUBE_WIDTH), static_cast<float>(VOXEL_CUBE_HEIGHT), static_cast<float>(VOXEL_CUBE_DEPTH));
    
    clear_voxels_textures();
    
    std::array<glm::vec3, 3> cam_positions = {  glm::vec3(0.0f, 0.0f, -8.f),glm::vec3(0.0f, 8.f, 0.0f), glm::vec3(8.0f, 0.0f, 0.0f)};
    std::array<glm::vec3, 3> up_vectors = { glm::vec3 {0.0f, 1.0f, 0.0f}, glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)};
    std::array<VkSemaphore, 3> semaphores = { _generate_voxel_z_axis_semaphore,
        _generate_voxel_y_axis_semaphore, _generate_voxel_x_axis_semaphore};
    
    glm::mat4 project_to_voxel_screen = glm::mat4(1.0f);
    
    //voxelize, we are going to build the voxel texture from cam_positions.size() views.
    for( size_t i = 0; i < 3; ++i)
    {
        _ortho_camera.position = cam_positions[i];
        _ortho_camera.forward = -_ortho_camera.position;
        _ortho_camera.up = up_vectors[i];
        _ortho_camera.update_view_matrix();
        
        if( i == 0)
        {
            deferred_output_params["vox_view_projection"] = _ortho_camera.get_projection_matrix() * _ortho_camera.view_matrix;
            deferred_output_params["eye_in_world_space"] = camera.position;
        }
        
        project_to_voxel_screen = (i == 0) ? _ortho_camera.get_projection_matrix() * _ortho_camera.view_matrix : project_to_voxel_screen;
        voxelize_frag_params["inverse_view_projection"] = glm::inverse( _ortho_camera.get_projection_matrix() * _ortho_camera.view_matrix);
        voxelize_frag_params["project_to_voxel_screen"] = project_to_voxel_screen;
        voxelize_vertex_params["view"] = _ortho_camera.view_matrix;
        voxelize_vertex_params["projection"] =_ortho_camera.get_projection_matrix();

        voxelize_vertex_params["light_position"] = _light_pos;
        voxelize_vertex_params["eye_position"] = camera.position;
        _voxelize_pipeline._material->commit_parameters_to_gpu();
        
       
        //TODO: it might be possible here to bulk all of these submissions into one
        VkSubmitInfo submit_info = {};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.waitSemaphoreCount =  1;
        submit_info.pWaitSemaphores = i == 0 ? &_clear_voxel_textures : &semaphores[i - 1];
        submit_info.pCommandBuffers = &(_voxelize_command_buffers[_deferred_image_index]);
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores = &semaphores[i];
        
        vkResetFences(_device->_logical_device, 1, &_voxelize_inflight_fence[_deferred_image_index]);
        vkQueueSubmit(_device->_graphics_queue, 1, &submit_info, _voxelize_inflight_fence[_deferred_image_index]);

        vkWaitForFences(_device->_logical_device, 1, &_voxelize_inflight_fence[_deferred_image_index], VK_TRUE, std::numeric_limits<uint64_t>::max());
    }
    
    generate_voxel_mip_maps();
}


void deferred_renderer::draw(camera& camera)
{
    
    vk::shader_parameter::shader_params_group& display_fragment_params = _pipeline._material->get_uniform_parameters(vk::visual_material::parameter_stage::FRAGMENT, 5) ;
    
    display_fragment_params["world_cam_position"] = glm::vec4(camera.position, 1.0f);
    display_fragment_params["world_light_position"] = _light_pos;
    display_fragment_params["light_color"] = _light_color;
    display_fragment_params["state"] = static_cast<int>(_rendering_state);
    
    int binding = 0;
    vk::shader_parameter::shader_params_group& display_params =   renderer::get_material()->get_uniform_parameters(vk::visual_material::parameter_stage::VERTEX, binding);
    
    display_params["width"] = static_cast<float>(_swapchain->_swapchain_data.swapchain_extent.width);
    display_params["height"] = static_cast<float>(_swapchain->_swapchain_data.swapchain_extent.height);
    
    perform_final_drawing_setup();
    
    //todo: acquire image at the very last minute, not in the very beginning
    vkAcquireNextImageKHR(_device->_logical_device, _swapchain->_swapchain_data.swapchain,
                          std::numeric_limits<uint64_t>::max(), _semaphore_image_available, VK_NULL_HANDLE, &_deferred_image_index);
    
    generate_voxel_textures(camera);
    
    std::array<VkSemaphore, 2> wait_semaphores{_semaphore_image_available, _g_buffers_rendering_done};
    //render g-buffers
    VkResult result = {};
    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = nullptr;
    submit_info.waitSemaphoreCount = wait_semaphores.size();
    submit_info.pWaitSemaphores = wait_semaphores.data();
    VkPipelineStageFlags wait_stage_mask[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submit_info.pWaitDstStageMask = wait_stage_mask;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &(_offscreen_command_buffers[_deferred_image_index]);
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &_g_buffers_rendering_done;

    vkResetFences(_device->_logical_device, 1, &_g_buffers_fence[_deferred_image_index]);
    result = vkQueueSubmit(_device->_graphics_queue, 1, &submit_info, _g_buffers_fence[_deferred_image_index]);
    vkWaitForFences(_device->_logical_device, 1, &_g_buffers_fence[_deferred_image_index], VK_TRUE, std::numeric_limits<uint64_t>::max());
    
    ASSERT_VULKAN(result);
    //render scene with g buffers and 3d voxel texture


    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = nullptr;
    submit_info.waitSemaphoreCount = wait_semaphores.size();
    submit_info.pWaitSemaphores = wait_semaphores.data();
    submit_info.pWaitDstStageMask = wait_stage_mask;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &(_command_buffers[_deferred_image_index]);
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &_semaphore_rendering_done;

    vkResetFences(_device->_logical_device, 1, &_composite_fence[_deferred_image_index]);
    result = vkQueueSubmit(_device->_graphics_queue, 1, &submit_info, _composite_fence[_deferred_image_index]);
    vkWaitForFences(_device->_logical_device, 1, &_composite_fence[_deferred_image_index], VK_TRUE, std::numeric_limits<uint64_t>::max());

    ASSERT_VULKAN(result);
    //present the scene to viewer
    VkPresentInfoKHR present_info {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pNext = nullptr;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores =&_semaphore_rendering_done;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &_swapchain->_swapchain_data.swapchain;
    present_info.pImageIndices = &_deferred_image_index;
    present_info.pResults = nullptr;
    result = vkQueuePresentKHR(_device->_present_queue, &present_info);
    //todo: check to see if you can collapse the 3 vkQueueSubmit calls into one, per nvidia: https://devblogs.nvidia.com/vulkan-dos-donts/
    
    ASSERT_VULKAN(result);
}

void deferred_renderer::destroy()
{
    renderer::destroy();
    
    vkDestroySemaphore(_device->_logical_device, _deferred_semaphore_image_available, nullptr);
    vkDestroySemaphore(_device->_logical_device, _g_buffers_rendering_done, nullptr);
    vkDestroySemaphore(_device->_logical_device, _voxelize_semaphore, nullptr);
    vkDestroySemaphore(_device->_logical_device, _voxelize_semaphore_done, nullptr);

    vkDestroySemaphore(_device->_logical_device, _generate_voxel_x_axis_semaphore, nullptr);
    vkDestroySemaphore(_device->_logical_device, _generate_voxel_y_axis_semaphore, nullptr);
    vkDestroySemaphore(_device->_logical_device, _generate_voxel_z_axis_semaphore, nullptr);
    vkDestroySemaphore(_device->_logical_device, _clear_voxel_textures, nullptr);
    
    for( size_t i = 0; i < _mip_map_semaphores.size(); ++i)
    {
        vkDestroySemaphore(_device->_logical_device, _mip_map_semaphores[i], nullptr);
    }
    
    _deferred_semaphore_image_available = VK_NULL_HANDLE;
    _g_buffers_rendering_done = VK_NULL_HANDLE;
    
    vkFreeCommandBuffers(_device->_logical_device, _device->_graphics_command_pool,
                         static_cast<uint32_t>(_swapchain->_swapchain_data.image_set.get_image_count()), _offscreen_command_buffers);
    
    vkFreeCommandBuffers(_device->_logical_device, _device->_compute_command_pool,
                         static_cast<uint32_t>(_swapchain->_swapchain_data.image_set.get_image_count()), _voxelize_command_buffers);
  
    for(int i = 0; i < _clear_3d_texture_command_buffers.size(); ++i)
    {
        vkFreeCommandBuffers(_device->_logical_device, _device->_compute_command_pool,
                             static_cast<uint32_t>(_swapchain->_swapchain_data.image_set.get_image_count()), _clear_3d_texture_command_buffers[i]) ;
    }

    for(int i = 0; i < _genered_3d_mip_maps_commands.size(); ++i)
    {
        vkFreeCommandBuffers(_device->_logical_device, _device->_compute_command_pool,
                             static_cast<uint32_t>(_swapchain->_swapchain_data.image_set.get_image_count()), _genered_3d_mip_maps_commands[i]) ;
    }

    
    
    delete[] _offscreen_command_buffers;
    
    for (size_t i = 0; i < _swapchain_frame_buffers.size(); i++)
    {
        vkDestroyFence(_device->_logical_device, _g_buffers_fence[i], nullptr);
        vkDestroyFence(_device->_logical_device, _voxelize_inflight_fence[i], nullptr);
        _g_buffers_fence[i] = VK_NULL_HANDLE;
    }
    
    _mrt_pipeline.destroy();
    _voxelize_pipeline.destroy();
    
    for( size_t i  = 0; i < _voxel_normal_textures.size(); ++i)
    {
        _voxel_albedo_textures[i].destroy();
        _voxel_normal_textures[i].destroy();
        _clear_voxel_texture_pipeline[i].destroy();
    }

    for( size_t i = 0; i < _create_voxel_mip_maps_pipelines.size(); ++i)
    {
        _create_voxel_mip_maps_pipelines[i].destroy();
    }

    
    vkDestroyRenderPass(_device->_logical_device, _mrt_render_pass, nullptr);
    vkDestroyRenderPass(_device->_logical_device, _voxelization_render_pass, nullptr);
    _mrt_render_pass = VK_NULL_HANDLE;
}
