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

template<typename RENDER_TEXTURE_TYPE, uint32_t NUM_ATTACHMENTS>
renderer<RENDER_TEXTURE_TYPE, NUM_ATTACHMENTS>::renderer(device* device, GLFWwindow* window, glfw_swapchain* swapchain, visual_mat_shared_ptr material):
_pipeline(device, glm::vec2( swapchain->get_vk_swap_extent().width, swapchain->get_vk_swap_extent().height), material)
{
    _device = device;
    _window = window;
    _swapchain = swapchain;
    
    _pipeline.set_rendering_attachments(_swapchain->present_textures);
}

template<typename RENDER_TEXTURE_TYPE, uint32_t NUM_ATTACHMENTS>
void renderer<RENDER_TEXTURE_TYPE, NUM_ATTACHMENTS>::create_command_buffers(VkCommandBuffer** command_buffers, VkCommandPool command_pool)
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

template<typename RENDER_TEXTURE_TYPE, uint32_t NUM_ATTACHMENTS>
void renderer<RENDER_TEXTURE_TYPE, NUM_ATTACHMENTS>::create_semaphore(VkSemaphore& semaphore)
{
    VkSemaphoreCreateInfo semaphore_create_info {};
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphore_create_info.pNext = nullptr;
    semaphore_create_info.flags = 0;
    
    
    VkResult result = vkCreateSemaphore(_device->_logical_device, &semaphore_create_info, nullptr, &semaphore);
    ASSERT_VULKAN(result);
}

template<typename RENDER_TEXTURE_TYPE, uint32_t NUM_ATTACHMENTS>
void renderer<RENDER_TEXTURE_TYPE, NUM_ATTACHMENTS>::create_semaphores(std::array<VkSemaphore, glfw_swapchain::NUM_SWAPCHAIN_IMAGES>& semaphores)
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

template<typename RENDER_TEXTURE_TYPE, uint32_t NUM_ATTACHMENTS>
void renderer<RENDER_TEXTURE_TYPE, NUM_ATTACHMENTS>::create_fence(VkFence& fence)
{
    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    
    VkResult result = vkCreateFence(_device->_logical_device, &fenceInfo, nullptr, &fence);
    ASSERT_VULKAN(result);
    
}
template<typename RENDER_TEXTURE_TYPE, uint32_t NUM_ATTACHMENTS>
void renderer<RENDER_TEXTURE_TYPE, NUM_ATTACHMENTS>::create_semaphores_and_fences()
{
    create_semaphore(_semaphore_image_available);
    create_semaphore(_semaphore_rendering_done);
    
    for(int i = 0; i < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++i)
    {
        create_fence(_composite_fence[i]);
    }
}

template<typename RENDER_TEXTURE_TYPE, uint32_t NUM_ATTACHMENTS>
void renderer<RENDER_TEXTURE_TYPE, NUM_ATTACHMENTS>::recreate_renderer()
{
    _device->wait_for_all_operations_to_finish();
    _pipeline.destroy();
    _swapchain->recreate_swapchain( );
    
    _pipeline.create();
    
    create_command_buffers(&_command_buffers, _device->_present_command_pool);
    record_command_buffers(_shapes.data(), _shapes.size());
    
}

template<typename RENDER_TEXTURE_TYPE, uint32_t NUM_ATTACHMENTS>
void renderer<RENDER_TEXTURE_TYPE, NUM_ATTACHMENTS>::record_command_buffers(obj_shape** shapes, size_t number_of_shapes)
{
    for (uint32_t i = 0; i < glfw_swapchain::NUM_SWAPCHAIN_IMAGES;i++)
    {
        _pipeline.begin_command_recording(_command_buffers[i], i);
        
        for( uint32_t j = 0; j < number_of_shapes; ++j)
        {
            shapes[j]->draw(_command_buffers[i], _pipeline, j, i);
        }
        
        _pipeline.end_command_recording();
    }
}

template<typename RENDER_TEXTURE_TYPE, uint32_t NUM_ATTACHMENTS>
void renderer<RENDER_TEXTURE_TYPE, NUM_ATTACHMENTS>::destroy()
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
}

template<typename RENDER_TEXTURE_TYPE, uint32_t NUM_ATTACHMENTS>
void renderer<RENDER_TEXTURE_TYPE, NUM_ATTACHMENTS>::init()
{
    assert(_shapes.size() != 0 && "add meshes to render before calling init");
    create_semaphores_and_fences();
    create_command_buffers(&_command_buffers, _device->_present_command_pool);
    
    assert(_shapes.size() != 0);
}

template<typename RENDER_TEXTURE_TYPE, uint32_t NUM_ATTACHMENTS>
void renderer<RENDER_TEXTURE_TYPE, NUM_ATTACHMENTS>::perform_final_drawing_setup()
{
    if(!_pipeline_created)
    {
        //note: we create the pipeline here to give the client a chance to set the material input arguments.
        //the pipeline needs this information to be created properly.

        _pipeline.create();
        record_command_buffers(_shapes.data(), _shapes.size());
        _pipeline_created = true;
    }
    _pipeline.commit_parameters_to_gpu(_image_index);
}

template<typename RENDER_TEXTURE_TYPE, uint32_t NUM_ATTACHMENTS>
void renderer<RENDER_TEXTURE_TYPE, NUM_ATTACHMENTS>::draw(camera& camera)
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

template<typename RENDER_TEXTURE_TYPE, uint32_t NUM_ATTACHMENTS>
renderer<RENDER_TEXTURE_TYPE, NUM_ATTACHMENTS>::~renderer()
{
}
