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


template<uint32_t NUM_ATTACHMENTS>
renderer<NUM_ATTACHMENTS>::renderer(device* device, GLFWwindow* window, glfw_swapchain* swapchain, material_store& store, const char* material_name):
_render_pass(device, glm::vec2( swapchain->get_vk_swap_extent().width, swapchain->get_vk_swap_extent().height))
{
    _device = device;
    _window = window;
    _swapchain = swapchain;
    
    _render_pass.get_attachment_group().add_attachment( _swapchain->present_textures);
    typename render_pass_type::subpass_s& subpass = _render_pass.add_subpass(store, material_name);
    
    static constexpr int ATTACHMENT_ID = 0;
    subpass.add_output_attachment(ATTACHMENT_ID);
}

template<uint32_t NUM_ATTACHMENTS>
void renderer<NUM_ATTACHMENTS>::create_command_buffers(VkCommandBuffer** command_buffers, VkCommandPool command_pool)
{
    VkCommandBufferAllocateInfo command_buffer_allocate_info {};
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

template<uint32_t NUM_ATTACHMENTS>
void renderer<NUM_ATTACHMENTS>::create_semaphore(VkSemaphore& semaphore)
{
    VkSemaphoreCreateInfo semaphore_create_info {};
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphore_create_info.pNext = nullptr;
    semaphore_create_info.flags = 0;
    
    
    VkResult result = vkCreateSemaphore(_device->_logical_device, &semaphore_create_info, nullptr, &semaphore);
    ASSERT_VULKAN(result);
}

template<uint32_t NUM_ATTACHMENTS>
void renderer<NUM_ATTACHMENTS>::create_semaphores(eastl::array<VkSemaphore, glfw_swapchain::NUM_SWAPCHAIN_IMAGES>& semaphores)
{
    for( int i = 0; i < semaphores.size(); ++i)
    {
        VkSemaphoreCreateInfo semaphore_create_info {};
        semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semaphore_create_info.pNext = nullptr;
        semaphore_create_info.flags = 0;
        
        
        VkResult result = vkCreateSemaphore(_device->_logical_device, &semaphore_create_info, nullptr, &semaphores[i]);
        ASSERT_VULKAN(result);

    }
}

template< uint32_t NUM_ATTACHMENTS>
void renderer< NUM_ATTACHMENTS>::create_fence(VkFence& fence)
{
    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    
    VkResult result = vkCreateFence(_device->_logical_device, &fenceInfo, nullptr, &fence);
    ASSERT_VULKAN(result);
    
}
template< uint32_t NUM_ATTACHMENTS>
void renderer< NUM_ATTACHMENTS>::create_semaphores_and_fences()
{
    
    create_semaphore(_semaphore_rendering_done);
    
    for(int i = 0; i < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++i)
    {
        create_semaphore(_semaphore_image_available[i]);
        create_fence(_composite_fence[i]);
    }
}

template<uint32_t NUM_ATTACHMENTS>
void renderer< NUM_ATTACHMENTS>::recreate_renderer()
{
    _device->wait_for_all_operations_to_finish();
    _render_pass.destroy();
    _swapchain->recreate_swapchain( );
    
    for(int i = 0; i< glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++i)
        _render_pass.create(i);
    
    create_command_buffers(&_command_buffers, _device->_present_command_pool);
    for( int i = 0; i < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++i)
    {
        record_command_buffers(_command_buffers[i],i );
    }
}

template<uint32_t NUM_ATTACHMENTS>
void renderer< NUM_ATTACHMENTS>::record_command_buffers(VkCommandBuffer& buffer, uint32_t swapchain_id)
{
    _render_pass.record_draw_commands(buffer, swapchain_id);
}

template< uint32_t NUM_ATTACHMENTS>
void renderer< NUM_ATTACHMENTS>::destroy()
{
    _device->wait_for_all_operations_to_finish();
    
    for( int i = 0; i < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++i)
    {
        vkDestroySemaphore(_device->_logical_device, _semaphore_image_available[i], nullptr);
        _semaphore_image_available[i] = VK_NULL_HANDLE;
    }

    vkDestroySemaphore(_device->_logical_device, _semaphore_rendering_done, nullptr);
    _semaphore_rendering_done = VK_NULL_HANDLE;
    
    vkFreeCommandBuffers(_device->_logical_device, _device->_graphics_command_pool,
                         glfw_swapchain::NUM_SWAPCHAIN_IMAGES, _command_buffers);
    delete[] _command_buffers;
    
    for (size_t i = 0; i < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; i++)
    {
        vkDestroyFence(_device->_logical_device, _composite_fence[i], nullptr);
        _composite_fence[i] = VK_NULL_HANDLE;
    }
    
    _render_pass.destroy();
}

template< uint32_t NUM_ATTACHMENTS>
void renderer< NUM_ATTACHMENTS>::init()
{
    create_semaphores_and_fences();
    create_command_buffers(&_command_buffers, _device->_present_command_pool);
}

template< uint32_t NUM_ATTACHMENTS>
void renderer< NUM_ATTACHMENTS>::perform_final_drawing_setup(VkCommandBuffer& buffer, uint32_t swapchain_id)
{
    _render_pass.commit_parameters_to_gpu(swapchain_id);
    _render_pass.record_draw_commands(buffer, swapchain_id);
}

template< uint32_t NUM_ATTACHMENTS>
void renderer< NUM_ATTACHMENTS>::draw(camera& camera)
{
    
    uint32_t current_index = _image_index;
    vkAcquireNextImageKHR(_device->_logical_device, _swapchain->get_vk_swapchain(),
                          std::numeric_limits<uint64_t>::max(),
                          _semaphore_image_available[current_index], VK_NULL_HANDLE, &_image_index);
    
    vkWaitForFences(_device->_logical_device, 1, &_composite_fence[_image_index], VK_TRUE, std::numeric_limits<uint64_t>::max());
    
    vkResetFences(_device->_logical_device, 1, &_composite_fence[_image_index]);
    perform_final_drawing_setup(_command_buffers[_image_index],_image_index);
    
    VkSubmitInfo submit_info;
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = nullptr;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &_semaphore_image_available[current_index];
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
    
    _next_frame = (_image_index + 1) % glfw_swapchain::NUM_SWAPCHAIN_IMAGES;
}

template< uint32_t NUM_ATTACHMENTS>
renderer< NUM_ATTACHMENTS>::~renderer()
{
}
