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
renderer(device, window, swapchain, store.GET_MAT<material>("deferred_output")),
_positions(device, swapchain->_swapchain_data.swapchain_extent.width, swapchain->_swapchain_data.swapchain_extent.height, render_texture::usage::COLOR_TARGET),
_albedo(device, swapchain->_swapchain_data.swapchain_extent.width,swapchain->_swapchain_data.swapchain_extent.height, render_texture::usage::COLOR_TARGET),
_normals(device, swapchain->_swapchain_data.swapchain_extent.width, swapchain->_swapchain_data.swapchain_extent.height, render_texture::usage::COLOR_TARGET),
_depth(device, swapchain->_swapchain_data.swapchain_extent.width,swapchain->_swapchain_data.swapchain_extent.height, render_texture::usage::DEPTH_TARGET),
_mrt_pipeline(device),
_plane(device),
_debug_pipeline(device)
{
    _mrt_material = store.GET_MAT<material>("mrt");
    
    int binding = 0;
    _mrt_material->init_parameter("model", material::parameter_stage::VERTEX, glm::mat4(0), binding);
    _mrt_material->init_parameter("view", material::parameter_stage::VERTEX, glm::mat4(0), binding);
    _mrt_material->init_parameter("projection", material::parameter_stage::VERTEX, glm::mat4(0), binding);
    _mrt_material->init_parameter("lightPosition", material::parameter_stage::VERTEX, glm::vec4(0), binding);
    
    _material->init_parameter("width", material::parameter_stage::VERTEX, 0.f, 0);
    _material->init_parameter("height", material::parameter_stage::VERTEX, 0.f, 0);
    
    _plane.create();
    create_command_buffer(&_offscreen_command_buffers);
}


void deferred_renderer::create_render_pass()
{
    //this call will create the swap chain render pass, or the render pass that will display stuff
    //to screen
    renderer::create_render_pass();
    
    //note: for an exellent explanation of attachments, go here:
    //https://stackoverflow.com/questions/46384007/vulkan-what-is-the-meaning-of-attachment
    
    std::array<VkAttachmentDescription, 4> attachment_descriptions {};
    
    for( uint32_t i = 0; i < attachment_descriptions.size(); ++i)
    {
        attachment_descriptions[i].samples = VK_SAMPLE_COUNT_1_BIT;
        attachment_descriptions[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment_descriptions[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment_descriptions[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment_descriptions[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        
        attachment_descriptions[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachment_descriptions[i].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        
        if (i == attachment_descriptions.size() -1)
        {
            attachment_descriptions[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachment_descriptions[i].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        }
    }
    
    //attachment_descriptions[0].format = static_cast<VkFormat>(_positions._format);
    attachment_descriptions[0].format = static_cast<VkFormat>(_normals._format);
    attachment_descriptions[1].format = static_cast<VkFormat>(_albedo._format);
    attachment_descriptions[2].format = static_cast<VkFormat>(_positions._format);
    attachment_descriptions[3].format = static_cast<VkFormat>(_depth._format);
    
    std::array<VkAttachmentReference, 3> color_references {};
    //note: the first integer in the instruction is the attachment number or location in the shader
    color_references[0] = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    color_references[1] = { 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    color_references[2] = { 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    //color_references[3] = { 3, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
    
    //todo: do I really need this depth attachment? we are rendering to textures
    VkAttachmentReference depth_reference = {};
    
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
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    
    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
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
    
    for(int i = 0; i < _swapchain->_swapchain_data.image_set.get_image_count(); ++i)
    {
        create_fence(_deferred_inflight_fences[i]);
    }
}

void deferred_renderer::create_pipeline()
{
    _pipeline.create(_render_pass,
                     _swapchain->_swapchain_data.swapchain_extent.width,
                     _swapchain->_swapchain_data.swapchain_extent.height);
    
    _mrt_pipeline.set_number_of_blend_attachments(3);
    bool enable_blend = false;
    _mrt_pipeline.modify_blend_attachment(0, pipeline::write_channels::RGBA, enable_blend);
    _mrt_pipeline.modify_blend_attachment(1, pipeline::write_channels::RGBA, enable_blend);
    _mrt_pipeline.modify_blend_attachment(2, pipeline::write_channels::RGBA, enable_blend);

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
        
        _pipeline.set_image_sampler(static_cast<texture_2d*>(&_normals), "normals", material::parameter_stage::FRAGMENT, 1);
        _pipeline.set_image_sampler(static_cast<texture_2d*>(&_albedo), "albedo", material::parameter_stage::FRAGMENT, 2);
        _pipeline.set_image_sampler(static_cast<texture_2d*>(&_depth), "positions", material::parameter_stage::FRAGMENT, 3);
        
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
    
    //POPULATE G-BUFFERS
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
    
    
    VkResult result = vkQueueSubmit(_device->_presentQueue, 1, &submit_info, _deferred_inflight_fences[image_index]);
    ASSERT_VULKAN(result);
    
    //RENDER SCENE WITH G-BUFFERS
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
    
    VkPresentInfoKHR present_info;
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pNext = nullptr;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &_semaphore_rendering_done;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &_swapchain->_swapchain_data.swapchain;
    present_info.pImageIndices = &image_index;
    present_info.pResults = nullptr;
    result = vkQueuePresentKHR(_device->_presentQueue, &present_info);
    
    ASSERT_VULKAN(result);
    
    int binding = 0;
    vk::shader_parameter::shader_params_group& display_params =   renderer::get_material()->get_uniform_parameters(vk::material::parameter_stage::VERTEX, binding);

    display_params["width"] = static_cast<float>(_swapchain->_swapchain_data.swapchain_extent.width);
    display_params["height"] = static_cast<float>(_swapchain->_swapchain_data.swapchain_extent.height);
    
    renderer::draw();
}

void deferred_renderer::destroy()
{
    renderer::destroy();
    
    vkDestroySemaphore(_device->_logical_device, _deferred_semaphore_image_available, nullptr);
    vkDestroySemaphore(_device->_logical_device, _deferred_semaphore_rendering_done, nullptr);
    _deferred_semaphore_image_available = VK_NULL_HANDLE;
    _deferred_semaphore_rendering_done = VK_NULL_HANDLE;

    vkFreeCommandBuffers(_device->_logical_device, _device->_commandPool,
                         static_cast<uint32_t>(_swapchain->_swapchain_data.image_set.get_image_count()), _offscreen_command_buffers);
    
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
