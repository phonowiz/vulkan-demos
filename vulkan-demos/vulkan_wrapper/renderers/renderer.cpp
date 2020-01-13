//
//  renderer.cpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 2/7/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "renderer.h"
#include "device.h"
#include "glfw_swapchain.h"
#include "depth_texture.h"
#include "vertex.h"
#include "vulkan_wrapper/shapes/obj_shape.h"
#include "shader.h"
#include "device.h"
#include <chrono>
#include <algorithm>
#include <stdio.h>

using namespace vk;


renderer::renderer(device* device, GLFWwindow* window, glfw_swapchain* swapchain, visual_mat_shared_ptr material):
_render_pass(device, false, glm::vec2(swapchain->get_vk_swap_extent().width, swapchain->get_vk_swap_extent().height)),
_pipeline(device, material)
{
    _device = device;
    _window = window;
    _swapchain = swapchain;
}

void renderer::create_render_pass()
{
    _render_pass.set_rendering_attachments(_swapchain->present_textures);
    _render_pass.init();
}


void renderer::create_command_buffers(VkCommandBuffer** command_buffers, VkCommandPool command_pool)
{
    VkCommandBufferAllocateInfo command_buffer_allocate_info;
    command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.pNext = nullptr;
    command_buffer_allocate_info.commandPool = command_pool;
    command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_allocate_info.commandBufferCount = glfw_swapchain::NUM_SWAPCHAIN_IMAGES;
    
    //TODO: remove "new"
    delete[] *command_buffers;
    *command_buffers = new VkCommandBuffer[glfw_swapchain::NUM_SWAPCHAIN_IMAGES];
    
    VkResult result = vkAllocateCommandBuffers(_device->_logical_device, &command_buffer_allocate_info, *command_buffers);
    ASSERT_VULKAN(result);
}



void renderer::create_semaphore(VkSemaphore& semaphore)
{
    VkSemaphoreCreateInfo semaphore_create_info {};
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphore_create_info.pNext = nullptr;
    semaphore_create_info.flags = 0;
    
    
    VkResult result = vkCreateSemaphore(_device->_logical_device, &semaphore_create_info, nullptr, &semaphore);
    ASSERT_VULKAN(result);
}

void renderer::create_fence(VkFence& fence)
{
    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    
    VkResult result = vkCreateFence(_device->_logical_device, &fenceInfo, nullptr, &fence);
    ASSERT_VULKAN(result);
    
}
void renderer::create_semaphores_and_fences()
{
    create_semaphore(_semaphore_image_available);
    create_semaphore(_semaphore_rendering_done);
    
    for(int i = 0; i < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++i)
    {
        create_fence(_composite_fence[i]);
    }
}

void renderer::recreate_renderer()
{
    
    _device->wait_for_all_operations_to_finish();
    
    _render_pass.destroy();
    _swapchain->recreate_swapchain( );
    
    _render_pass.init();
    
    create_command_buffers(&_command_buffers, _device->_present_command_pool);
    record_command_buffers(_shapes.data(), _shapes.size());
    
}

void renderer::record_command_buffers(obj_shape** shapes, size_t number_of_shapes)
{
    VkCommandBufferBeginInfo command_buffer_begin_info {};
    command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_begin_info.pNext = nullptr;
    command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    command_buffer_begin_info.pInheritanceInfo = nullptr;
    
    for (uint32_t i = 0; i < glfw_swapchain::NUM_SWAPCHAIN_IMAGES;i++)
    {
        VkResult result = vkBeginCommandBuffer(_command_buffers[i], &command_buffer_begin_info);
        ASSERT_VULKAN(result);
        
        VkRenderPassBeginInfo render_pass_create_info = {};
        render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_create_info.pNext = nullptr;
        render_pass_create_info.renderPass = _render_pass.get_vk_render_pass(i);
        render_pass_create_info.framebuffer = _render_pass.get_vk_frame_buffer(i);
        render_pass_create_info.renderArea.offset = { 0, 0 };
        render_pass_create_info.renderArea.extent = { _swapchain->get_vk_swap_extent().width,
            _swapchain->get_vk_swap_extent().height };
        VkClearValue clear_value = {0.0f, 0.0f, 0.0f, 1.0f};
        VkClearValue depth_clear_value = {1.0f, 0.0f};
        
        std::array<VkClearValue,2> clear_values;
        clear_values[0] = clear_value;
        clear_values[1] = depth_clear_value;
        
        render_pass_create_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
        render_pass_create_info.pClearValues = clear_values.data();
        
        vkCmdBeginRenderPass(_command_buffers[i], &render_pass_create_info, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(_command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline._pipeline[i]);
        
        VkViewport viewport;
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport. width = _swapchain->get_vk_swap_extent().width;
        viewport.height = _swapchain->get_vk_swap_extent().height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(_command_buffers[i], 0, 1, &viewport);
        
        VkRect2D scissor;
        scissor.offset = { 0, 0};
        scissor.extent = { _swapchain->get_vk_swap_extent().width,
            _swapchain->get_vk_swap_extent().height};
        vkCmdSetScissor(_command_buffers[i], 0, 1, &scissor);
        
        
        for( uint32_t j = 0; j < number_of_shapes; ++j)
        {
            shapes[j]->draw(_command_buffers[i], _pipeline, j, i);
        }
        
        vkCmdEndRenderPass(_command_buffers[i]);
        
        result = vkEndCommandBuffer(_command_buffers[i]);
        ASSERT_VULKAN(result);
    }
}

void renderer::destroy()
{
    _device->wait_for_all_operations_to_finish();
    vkDestroySemaphore(_device->_logical_device, _semaphore_image_available, nullptr);
    vkDestroySemaphore(_device->_logical_device, _semaphore_rendering_done, nullptr);
    _semaphore_image_available = VK_NULL_HANDLE;
    _semaphore_rendering_done = VK_NULL_HANDLE;
    
    vkFreeCommandBuffers(_device->_logical_device, _device->_graphics_command_pool,
                         glfw_swapchain::NUM_SWAPCHAIN_IMAGES, _command_buffers);
    delete[] _command_buffers;
    
    for (size_t i = 0; i < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; i++)
    {
        vkDestroyFence(_device->_logical_device, _composite_fence[i], nullptr);
        _composite_fence[i] = VK_NULL_HANDLE;
    }
    
    _pipeline.destroy();
    _render_pass.destroy();
}

void renderer::create_pipeline()
{
    _pipeline.create(_render_pass, _swapchain->get_vk_swap_extent().width, _swapchain->get_vk_swap_extent().height);
}

void renderer::init()
{
    assert(_shapes.size() != 0 && "add meshes to render before calling init");
    create_render_pass();
    create_semaphores_and_fences();
    create_command_buffers(&_command_buffers, _device->_present_command_pool);
    
    assert(_shapes.size() != 0);
    
}

void renderer::perform_final_drawing_setup()
{
    if(!_pipeline_created)
    {
        //note: we create the pipeline here to give the client a chance to set the material input arguments.
        //the pipeline needs this information to be created properly.
        create_pipeline();
        record_command_buffers(_shapes.data(), _shapes.size());
        _pipeline_created = true;
    }
    _pipeline.commit_parameters_to_gpu(_image_index);
}

#include <iostream>
void renderer::draw(camera& camera)
{
    vkWaitForFences(_device->_logical_device, 1, &_composite_fence[_image_index], VK_TRUE, std::numeric_limits<uint64_t>::max());
    vkResetFences(_device->_logical_device, 1, &_composite_fence[_image_index]);
    
    vkAcquireNextImageKHR(_device->_logical_device, _swapchain->get_vk_swapchain(),
                          std::numeric_limits<uint64_t>::max(), _semaphore_image_available, VK_NULL_HANDLE, &_image_index);
    
    perform_final_drawing_setup();
    
    VkSubmitInfo submit_info;
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = nullptr;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &_semaphore_image_available;
    VkPipelineStageFlags wait_stage_mask[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submit_info.pWaitDstStageMask = wait_stage_mask;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &(_command_buffers[_image_index]);
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &_semaphore_rendering_done;
    
    VkResult result = vkQueueSubmit(_device->_graphics_queue, 1, &submit_info, _composite_fence[_image_index]);
    ASSERT_VULKAN(result);
    
    VkPresentInfoKHR present_info;
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pNext = nullptr;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &_semaphore_rendering_done;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &_swapchain->get_vk_swapchain();
    present_info.pImageIndices = &_image_index;
    present_info.pResults = nullptr;
    result = vkQueuePresentKHR(_device->_present_queue, &present_info);
    
    ASSERT_VULKAN(result);
}

renderer::~renderer()
{
}
