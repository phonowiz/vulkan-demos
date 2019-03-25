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
#include "swapchain.h"
#include "depth_image.h"
#include "vertex.h"
#include "vulkan_wrapper/mesh.h"
#include "shader.h"
#include "device.h"
#include <chrono>
#include <algorithm>
#include <stdio.h>

using namespace vk;


renderer::renderer(device* device, GLFWwindow* window, swapchain* swapChain, material_shared_ptr material):
_depth_image(device),
_pipeline(device, material)
{
    memset(_attachments.data(), NULL, _attachments.size() * sizeof(texture_2d*));
    _device = device;
    _window = window;
    _swapchain = swapChain;
    _material = material;
}

void renderer::create_render_pass()
{
    
    uint32_t num_attachments = 1;
    std::array<VkAttachmentDescription, MAX_ATTACHMENTS> attachment_descriptions;
    attachment_descriptions[0].flags = 0;
    attachment_descriptions[0].format = _swapchain->get_surface_format().format;
    attachment_descriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachment_descriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment_descriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment_descriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment_descriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment_descriptions[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment_descriptions[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    
    for( int i = 0; i < MAX_ATTACHMENTS; ++i)
    {
        if(_attachments[i - 1] != nullptr)
        {
            attachment_descriptions[i].flags = 0;
            attachment_descriptions[i].format = static_cast<VkFormat>(_attachments[i - 1]->_format);
            attachment_descriptions[i].samples = VK_SAMPLE_COUNT_1_BIT;
            attachment_descriptions[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachment_descriptions[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachment_descriptions[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachment_descriptions[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachment_descriptions[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachment_descriptions[i].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            
            ++num_attachments;
        }
    }
    
    attachment_descriptions[num_attachments] = _depth_image.get_depth_attachment();
    ++num_attachments;
    
    VkAttachmentReference attachment_reference;
    attachment_reference.attachment = 0;
    attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    VkAttachmentReference depth_attachment_reference;
    depth_attachment_reference.attachment = 1;
    depth_attachment_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    
    VkSubpassDescription subpass_description;
    subpass_description.flags = 0;
    subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_description.inputAttachmentCount = 0;
    subpass_description.pInputAttachments = nullptr;
    subpass_description.colorAttachmentCount = 1;
    subpass_description.pColorAttachments = &attachment_reference;
    subpass_description.pResolveAttachments = nullptr;
    subpass_description.pDepthStencilAttachment = &depth_attachment_reference;
    subpass_description.preserveAttachmentCount = 0;
    subpass_description.pPreserveAttachments = nullptr;
    
    VkSubpassDependency subpass_dependency;
    subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpass_dependency.dstSubpass = 0;
    subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_dependency.srcAccessMask = 0;
    subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subpass_dependency.dependencyFlags = 0;
    

    VkRenderPassCreateInfo render_pass_create_info;
    render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_create_info.pNext = nullptr;
    render_pass_create_info.flags = 0;
    render_pass_create_info.attachmentCount = num_attachments;//static_cast<uint32_t>(_attachments.size());
    render_pass_create_info.pAttachments = attachment_descriptions.data();
    render_pass_create_info.subpassCount = 1;
    render_pass_create_info.pSubpasses = &subpass_description;
    render_pass_create_info.dependencyCount = 1;
    render_pass_create_info.pDependencies = &subpass_dependency;
    
    VkResult result = vkCreateRenderPass(_device->_logical_device, &render_pass_create_info, nullptr, &_render_pass);

    ASSERT_VULKAN(result);
}


void renderer::create_command_buffer()
{
    VkCommandBufferAllocateInfo command_buffer_allocate_info;
    command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.pNext = nullptr;
    command_buffer_allocate_info.commandPool = _device->_commandPool;
    command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_allocate_info.commandBufferCount = static_cast<uint32_t>(_swapchain->_swapchain_data.image_set.get_image_count());
    
    //todo: remove "new"
    _command_buffers = new VkCommandBuffer[_swapchain->_swapchain_data.image_set.get_image_count()];
    VkResult result = vkAllocateCommandBuffers(_device->_logical_device, &command_buffer_allocate_info, _command_buffers);
   ASSERT_VULKAN(result);
}




void renderer::create_semaphores()
{
    VkSemaphoreCreateInfo semaphore_create_info;
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphore_create_info.pNext = nullptr;
    semaphore_create_info.flags = 0;
    
    VkResult result = vkCreateSemaphore(_device->_logical_device, &semaphore_create_info, nullptr, &_semaphore_image_available);
    ASSERT_VULKAN(result);
    result = vkCreateSemaphore(_device->_logical_device, &semaphore_create_info, nullptr, &_semaphore_rendering_done);
    ASSERT_VULKAN(result);
    
    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    
    
    for(int i = 0; i < _swapchain->_swapchain_data.image_set.get_image_count(); ++i)
    {
        result = vkCreateFence(_device->_logical_device, &fenceInfo, nullptr, &_inflight_fences[i]);
        ASSERT_VULKAN(result);
    }
}

void renderer::recreate_renderer()
{

    _device->wait_for_all_operations_to_finish();

    vkDestroyRenderPass(_device->_logical_device, _render_pass, nullptr);

    create_render_pass();
    destroy_framebuffers();
    _swapchain->recreate_swapchain( );
    create_frame_buffers();
    
    create_command_buffer();
    record_command_buffers();

}

void renderer::record_command_buffers()
{
    VkCommandBufferBeginInfo command_buffer_begin_info;
    command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_begin_info.pNext = nullptr;
    command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    command_buffer_begin_info.pInheritanceInfo = nullptr;
    
    
    
    for (size_t i = 0; i < _swapchain->_swapchain_data.image_set.get_image_count(); i++)
    {
        VkResult result = vkBeginCommandBuffer(_command_buffers[i], &command_buffer_begin_info);
        ASSERT_VULKAN(result);
        
        VkRenderPassBeginInfo render_pass_create_info;
        render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_create_info.pNext = nullptr;
        render_pass_create_info.renderPass = _render_pass;
        render_pass_create_info.framebuffer = _swapchain_frame_buffers[i];
        render_pass_create_info.renderArea.offset = { 0, 0 };
        render_pass_create_info.renderArea.extent = { _swapchain->_swapchain_data.swapchain_extent.width, _swapchain->_swapchain_data.swapchain_extent.height };
        VkClearValue clearValue = {0.0f, 0.0f, 0.0f, 1.0f};
        VkClearValue depthClearValue = {1.0f, 0.0f};
        
        std::array<VkClearValue,2> clear_values;
        clear_values[0] = clearValue;
        clear_values[1] = depthClearValue;
        
        render_pass_create_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
        render_pass_create_info.pClearValues = clear_values.data();
        
        
        vkCmdBeginRenderPass(_command_buffers[i], &render_pass_create_info, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(_command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline._pipeline);
        
        VkViewport viewport;
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = _swapchain->_swapchain_data.swapchain_extent.width;
        viewport.height = _swapchain->_swapchain_data.swapchain_extent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(_command_buffers[i], 0, 1, &viewport);
        
        VkRect2D scissor;
        scissor.offset = { 0, 0};
        scissor.extent = { _swapchain->_swapchain_data.swapchain_extent.width, _swapchain->_swapchain_data.swapchain_extent.height};
        vkCmdSetScissor(_command_buffers[i], 0, 1, &scissor);

        
        for( vk::mesh* pMesh : _meshes)
        {
            pMesh->draw(_command_buffers[i], _pipeline);
        }
        
        vkCmdEndRenderPass(_command_buffers[i]);
        
        result = vkEndCommandBuffer(_command_buffers[i]);
        ASSERT_VULKAN(result);
    }
}

void renderer::destroy_framebuffers()
{
    for (size_t i = 0; i < _swapchain_frame_buffers.size(); i++)
    {
        vkDestroyFramebuffer(_device->_logical_device, _swapchain_frame_buffers[i], nullptr);
        _swapchain_frame_buffers[i] = VK_NULL_HANDLE;
    }
}
void renderer::destroy()
{
    vkDestroySemaphore(_device->_logical_device, _semaphore_image_available, nullptr);
    vkDestroySemaphore(_device->_logical_device, _semaphore_rendering_done, nullptr);
    _semaphore_image_available = VK_NULL_HANDLE;
    _semaphore_rendering_done = VK_NULL_HANDLE;
    
    vkFreeCommandBuffers(_device->_logical_device, _device->_commandPool,
                         static_cast<uint32_t>(_swapchain->_swapchain_data.image_set.get_image_count()), _command_buffers);
    delete[] _command_buffers;
    
    destroy_framebuffers();
    _depth_image.destroy();

    for (size_t i = 0; i < _swapchain_frame_buffers.size(); i++)
    {
        vkDestroyFence(_device->_logical_device, _inflight_fences[i], nullptr);
        _inflight_fences[i] = VK_NULL_HANDLE;
    }
    
    _pipeline.destroy();
    vkDestroyRenderPass(_device->_logical_device, _render_pass, nullptr);
    _render_pass = VK_NULL_HANDLE;
}

void renderer::create_frame_buffers()
{
    _depth_image.destroy();
    _depth_image.create(_swapchain->_swapchain_data.swapchain_extent.width, _swapchain->_swapchain_data.swapchain_extent.height);
    //TODO: Get rid of the vector class
    _swapchain_frame_buffers.resize(_swapchain->_swapchain_data.image_set.get_image_count());
    
    for (size_t i = 0; i < _swapchain_frame_buffers.size(); i++)
    {
        VkImageView depth_image_view = _depth_image.get_image_view();
        assert(depth_image_view != VK_NULL_HANDLE);
        std::array<VkImageView, 2> attachment_views = {_swapchain->_swapchain_data.image_set.get_image_views()[i], depth_image_view};
        
        VkFramebufferCreateInfo framebuffer_create_info;
        framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_create_info.pNext = nullptr;
        framebuffer_create_info.flags = 0;
        framebuffer_create_info.renderPass = _render_pass;
        framebuffer_create_info.attachmentCount = static_cast<uint32_t>(attachment_views.size());
        framebuffer_create_info.pAttachments = attachment_views.data();
        framebuffer_create_info.width = _swapchain->_swapchain_data.swapchain_extent.width;
        framebuffer_create_info.height = _swapchain->_swapchain_data.swapchain_extent.height;
        framebuffer_create_info.layers = 1;
        
        VkResult result = vkCreateFramebuffer(_device->_logical_device, &framebuffer_create_info, nullptr, &(_swapchain_frame_buffers[i]));
        ASSERT_VULKAN(result);
    }
}
void renderer::create_pipeline()
{
    _pipeline.create(_render_pass, _swapchain->_swapchain_data.swapchain_extent.width, _swapchain->_swapchain_data.swapchain_extent.height);
}

void renderer::init()
{
    create_render_pass();
    _material->create_descriptor_set_layout();
    _swapchain->recreate_swapchain();
    
    create_frame_buffers();
    create_pipeline();
    create_semaphores();
    create_command_buffer();
    
    //we only support 1 mesh at the moment
    assert(_meshes.size() == 1);
    assert(_meshes.size() != 0);
    
    for(int i = 0; i < _meshes.size(); ++i)
    {
        _meshes[i]->allocate_gpu_memory();
    }
    create_command_buffer();
    record_command_buffers();
    
}

void renderer::draw()
{
    static uint32_t image_index = 0;
    vkWaitForFences(_device->_logical_device, 1, &_inflight_fences[image_index], VK_TRUE, std::numeric_limits<uint64_t>::max());
    vkResetFences(_device->_logical_device, 1, &_inflight_fences[image_index]);
    
    vkAcquireNextImageKHR(_device->_logical_device, _swapchain->_swapchain_data.swapchain,
                          std::numeric_limits<uint64_t>::max(), _semaphore_image_available, VK_NULL_HANDLE, &image_index);
    
    VkSubmitInfo submit_info;
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = nullptr;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &_semaphore_image_available;
    VkPipelineStageFlags wait_stage_mask[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submit_info.pWaitDstStageMask = wait_stage_mask;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &(_command_buffers[image_index]);
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &_semaphore_rendering_done;
    
    VkResult result = vkQueueSubmit(_device->_graphics_queue, 1, &submit_info, _inflight_fences[image_index]);
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
}

renderer::~renderer()
{
}
