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
#include "mesh.h"

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
_depth(device, true),
_mrt_pipeline(device),
_plane(device),
_debug_pipeline(device),
_mrt_material(store.GET_MAT<visual_material>("mrt")),
_voxelize_pipeline(device, store.GET_MAT<compute_material>("voxelizer"))
{
    int binding = 0;
    _mrt_material->init_parameter("model", visual_material::parameter_stage::VERTEX, glm::mat4(0), binding);
    _mrt_material->init_parameter("view", visual_material::parameter_stage::VERTEX, glm::mat4(0), binding);
    _mrt_material->init_parameter("projection", visual_material::parameter_stage::VERTEX, glm::mat4(0), binding);
    _mrt_material->init_parameter("lightPosition", visual_material::parameter_stage::VERTEX, glm::vec4(0), binding);
    
    _material->init_parameter("width", visual_material::parameter_stage::VERTEX, 0.f, 0);
    _material->init_parameter("height", visual_material::parameter_stage::VERTEX, 0.f, 0);
    
    _voxelize_pipeline.set_image_sampler(&_depth, "depth_texture", 0, resource::usage_type::COMBINED_IMAGE_SAMPLER);
    _voxelize_pipeline.set_image_sampler(&_depth, "3d_texture", 1, resource::usage_type::STORAGE_IMAGE);
    
    //TODO: I think maybe material should be private and we should have a function that does this for us,
    //we can already set image samplers using the pipeline object.  Also, consider creating a frame buffer object which houses
    //the pipeline and rendering commands to be used for rendering meshes.
    
    _voxelize_pipeline.create();
    
    _depth.create(_swapchain->_swapchain_data.swapchain_extent.width, _swapchain->_swapchain_data.swapchain_extent.height);
    _plane.create();
    create_command_buffer(&_offscreen_command_buffers, _device->_graphics_command_pool);
    create_command_buffer(&_voxelize_command_buffers, _device->_compute_command_pool);
}


void deferred_renderer::create_render_pass()
{
    //this call will create the swap chain render pass, or the render pass that will display stuff
    //to screen
    renderer::create_render_pass();
    
    //note: for an exellent explanation of attachments, go here:
    //https://stackoverflow.com/questions/46384007/vulkan-what-is-the-meaning-of-attachment
    
    std::array<VkAttachmentDescription, 4> attachment_descriptions {};
    
    for( uint32_t i = 0; i < attachment_descriptions.size() - 2; ++i)
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
    render_pass_info.dependencyCount = 2;
    render_pass_info.pDependencies = dependencies.data();
    
    vkCreateRenderPass(_device->_logical_device, &render_pass_info, nullptr, &_mrt_render_pass);
}

void deferred_renderer::create_semaphores_and_fences()
{
    renderer::create_semaphores_and_fences();
    
    create_semaphore(_deferred_semaphore_image_available);
    create_semaphore(_deferred_semaphore_rendering_done);
    create_semaphore(_voxelize_semaphore);
    create_semaphore(_voxelize_semaphore_done);
    
    for(int i = 0; i < _swapchain->_swapchain_data.image_set.get_image_count(); ++i)
    {
        create_fence(_deferred_inflight_fences[i]);
        create_fence(_voxelize_inflight_fence[i]);
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
}


void deferred_renderer::create_frame_buffers()
{
    renderer::create_frame_buffers();
    
    //TODO: Get rid of the vector class
    _deferred_swapchain_frame_buffers.resize(_swapchain->_swapchain_data.image_set.get_image_count());
    
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
        ASSERT_VULKAN(result);
    }
}


void deferred_renderer::record_voxelize_command_buffers()
{
    assert(_voxelize_pipeline._pipeline != VK_NULL_HANDLE);
    assert(_voxelize_pipeline._pipeline_layout != VK_NULL_HANDLE);
    
    for( int i = 0; i < _swapchain->_swapchain_data.image_set.get_image_count(); ++i)
    {
        VkCommandBufferBeginInfo command_buffer_begin_info {};
        command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        
        vkBeginCommandBuffer(_voxelize_command_buffers[i], &command_buffer_begin_info);
        
        vkCmdBindPipeline(_voxelize_command_buffers[i], VK_PIPELINE_BIND_POINT_COMPUTE, _voxelize_pipeline._pipeline);
        vkCmdBindDescriptorSets(_voxelize_command_buffers[i], VK_PIPELINE_BIND_POINT_COMPUTE, _voxelize_pipeline._pipeline_layout, 0, 1,
                                _voxelize_pipeline._material->get_descriptor_set(), 0, VK_NULL_HANDLE);
        
        vkCmdDispatch(_voxelize_command_buffers[i], _swapchain->_swapchain_data.swapchain_extent.width/16,
                      _swapchain->_swapchain_data.swapchain_extent.height/16, 1);
        
        vkEndCommandBuffer(_voxelize_command_buffers[i]);
    }

}

void deferred_renderer::record_command_buffers(mesh* meshes, size_t number_of_meshes)
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
        
        VkViewport viewport;
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = _swapchain->_swapchain_data.swapchain_extent.width;
        viewport.height = _swapchain->_swapchain_data.swapchain_extent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(_offscreen_command_buffers[i], 0, 1, &viewport);
        
        VkRect2D scissor;
        scissor.offset = { 0, 0};
        scissor.extent = { _swapchain->_swapchain_data.swapchain_extent.width,
            _swapchain->_swapchain_data.swapchain_extent.height};
        vkCmdSetScissor(_offscreen_command_buffers[i], 0, 1, &scissor);
        
        for( size_t j = 0; j < number_of_meshes; ++j)
        {
            meshes[j].draw(_offscreen_command_buffers[i], _mrt_pipeline);
        }
        
        vkCmdEndRenderPass(_offscreen_command_buffers[i]);
    }
    _plane.allocate_gpu_memory();
    renderer::record_command_buffers(&_plane, 1);
}

void deferred_renderer::destroy_framebuffers()
{
    renderer::destroy_framebuffers();
    
    for (size_t i = 0; i < _swapchain_frame_buffers.size(); i++)
    {
        vkDestroyFramebuffer(_device->_logical_device, _deferred_swapchain_frame_buffers[i], nullptr);
        _deferred_swapchain_frame_buffers[i] = VK_NULL_HANDLE;
    }
}

void deferred_renderer::perform_final_drawing_setup()
{
    static bool setup_initialized = false;
    if( !setup_initialized)
    {
        _mrt_pipeline.set_material(_mrt_material);
        _pipeline.set_material(_material);
        //note: the last argument is the binding number specified in the material's shader
        _pipeline.set_image_sampler(static_cast<texture_2d*>(&_normals), "normals", visual_material::parameter_stage::FRAGMENT, 1, resource::usage_type::COMBINED_IMAGE_SAMPLER);
        _pipeline.set_image_sampler(static_cast<texture_2d*>(&_albedo), "albedo", visual_material::parameter_stage::FRAGMENT, 2, resource::usage_type::COMBINED_IMAGE_SAMPLER);
        _pipeline.set_image_sampler(static_cast<texture_2d*>(&_positions), "positions", visual_material::parameter_stage::FRAGMENT, 3, resource::usage_type::COMBINED_IMAGE_SAMPLER);
        _pipeline.set_image_sampler(static_cast<texture_2d*>(&_depth), "depth", visual_material::parameter_stage::FRAGMENT, 4, resource::usage_type::COMBINED_IMAGE_SAMPLER);
        
        setup_initialized = true;
    }
    _mrt_material->commit_parameters_to_gpu();
    
    renderer::perform_final_drawing_setup();
}

void deferred_renderer::draw()
{
    perform_final_drawing_setup();
    
    static uint32_t image_index = 0;
    vkWaitForFences(_device->_logical_device, 1, &_deferred_inflight_fences[image_index], VK_TRUE, std::numeric_limits<uint64_t>::max());
    vkResetFences(_device->_logical_device, 1, &_deferred_inflight_fences[image_index]);
    
    vkAcquireNextImageKHR(_device->_logical_device, _swapchain->_swapchain_data.swapchain,
                          std::numeric_limits<uint64_t>::max(), _semaphore_image_available, VK_NULL_HANDLE, &image_index);
    
    //render g-buffers
    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = nullptr;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &_semaphore_image_available;
    VkPipelineStageFlags wait_stage_mask[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submit_info.pWaitDstStageMask = wait_stage_mask;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &(_offscreen_command_buffers[image_index]);
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &_deferred_semaphore_rendering_done;
    
    
    VkResult result = vkQueueSubmit(_device->_graphics_queue, 1, &submit_info, _deferred_inflight_fences[image_index]);
    ASSERT_VULKAN(result);
    

    //Present g-buffers, in other words, save g buffers to textures, make sure we are done rendering them before we do
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = nullptr;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &_deferred_semaphore_rendering_done;
    submit_info.pWaitDstStageMask = wait_stage_mask;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &(_command_buffers[image_index]);
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &_semaphore_rendering_done;

    result = vkQueueSubmit(_device->_graphics_queue, 1, &submit_info, _inflight_fences[image_index]);
    ASSERT_VULKAN(result);
    
    //voxelize
    vkWaitForFences(_device->_logical_device, 1, &_voxelize_inflight_fence[image_index], VK_TRUE, std::numeric_limits<uint64_t>::max());
    vkResetFences(_device->_logical_device, 1, &_voxelize_inflight_fence[image_index]);
    
    submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &_semaphore_rendering_done;
    submit_info.pCommandBuffers = &(_voxelize_command_buffers[image_index]);
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &_voxelize_semaphore_done;
    vkResetCommandBuffer( _voxelize_command_buffers[image_index], 0);
    vkQueueSubmit(_device->_compute_queue, 1, &submit_info, _voxelize_inflight_fence[image_index]);
    
    //render scene with g buffers and 3d voxel texture
    int binding = 0;
    vk::shader_parameter::shader_params_group& display_params =   renderer::get_material()->get_uniform_parameters(vk::visual_material::parameter_stage::VERTEX, binding);
    
    display_params["width"] = static_cast<float>(_swapchain->_swapchain_data.swapchain_extent.width);
    display_params["height"] = static_cast<float>(_swapchain->_swapchain_data.swapchain_extent.height);
    
    VkPresentInfoKHR present_info {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pNext = nullptr;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &_voxelize_semaphore_done;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &_swapchain->_swapchain_data.swapchain;
    present_info.pImageIndices = &image_index;
    present_info.pResults = nullptr;
    result = vkQueuePresentKHR(_device->_present_queue, &present_info);

    ASSERT_VULKAN(result);
}

void deferred_renderer::destroy()
{
    renderer::destroy();
    
    vkDestroySemaphore(_device->_logical_device, _deferred_semaphore_image_available, nullptr);
    vkDestroySemaphore(_device->_logical_device, _deferred_semaphore_rendering_done, nullptr);
    vkDestroySemaphore(_device->_logical_device, _voxelize_semaphore, nullptr);
    vkDestroySemaphore(_device->_logical_device, _voxelize_semaphore_done, nullptr);
    
    _deferred_semaphore_image_available = VK_NULL_HANDLE;
    _deferred_semaphore_rendering_done = VK_NULL_HANDLE;

    vkFreeCommandBuffers(_device->_logical_device, _device->_graphics_command_pool,
                         static_cast<uint32_t>(_swapchain->_swapchain_data.image_set.get_image_count()), _offscreen_command_buffers);
    
    vkFreeCommandBuffers(_device->_logical_device, _device->_compute_command_pool,
                         static_cast<uint32_t>(_swapchain->_swapchain_data.image_set.get_image_count()), _voxelize_command_buffers);
    
    delete[] _offscreen_command_buffers;
    
    for (size_t i = 0; i < _swapchain_frame_buffers.size(); i++)
    {
        vkDestroyFence(_device->_logical_device, _deferred_inflight_fences[i], nullptr);
        _deferred_inflight_fences[i] = VK_NULL_HANDLE;
    }
    
    _mrt_pipeline.destroy();
    _debug_pipeline.destroy();
    
    vkDestroyRenderPass(_device->_logical_device, _mrt_render_pass, nullptr);
    _mrt_render_pass = VK_NULL_HANDLE;
}
