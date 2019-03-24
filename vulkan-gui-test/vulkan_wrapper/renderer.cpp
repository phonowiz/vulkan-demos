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


using namespace vk;


renderer::renderer(device* device, GLFWwindow* window, swapchain* swapChain, material_shared_ptr material):
_pipeline(device, material)
{
    _device = device;
    _window = window;
    _swapchain = swapChain;
    _material = material;
}

void renderer::create_render_pass()
{
    
    VkAttachmentDescription attachmentDescription;
    attachmentDescription.flags = 0;
    attachmentDescription.format = _swapchain->get_surface_format().format;
    attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
    attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    
    VkAttachmentReference attachmentReference;
    attachmentReference.attachment = 0;
    attachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    VkAttachmentDescription depthAttachment = _swapchain->get_depth_attachment();
    
    VkAttachmentReference depthAttachmentReference;
    depthAttachmentReference.attachment = 1;
    depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    
    VkSubpassDescription subpassDescription;
    subpassDescription.flags = 0;
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.inputAttachmentCount = 0;
    subpassDescription.pInputAttachments = nullptr;
    subpassDescription.colorAttachmentCount = 1;
    subpassDescription.pColorAttachments = &attachmentReference;
    subpassDescription.pResolveAttachments = nullptr;
    subpassDescription.pDepthStencilAttachment = &depthAttachmentReference;
    subpassDescription.preserveAttachmentCount = 0;
    subpassDescription.pPreserveAttachments = nullptr;
    
    VkSubpassDependency subpassDependency;
    subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependency.dstSubpass = 0;
    subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.srcAccessMask = 0;
    subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subpassDependency.dependencyFlags = 0;
    
    std::array<VkAttachmentDescription,2> attachments;
    attachments[0] = (attachmentDescription);
    attachments[1] = (depthAttachment);
    
    VkRenderPassCreateInfo renderPassCreateInfo;
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.pNext = nullptr;
    renderPassCreateInfo.flags = 0;
    renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassCreateInfo.pAttachments = attachments.data();
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpassDescription;
    renderPassCreateInfo.dependencyCount = 1;
    renderPassCreateInfo.pDependencies = &subpassDependency;
    
    VkResult result = vkCreateRenderPass(_device->_logical_device, &renderPassCreateInfo, nullptr, &_render_pass);

    ASSERT_VULKAN(result);
}


void renderer::create_command_buffer()
{
    VkCommandBufferAllocateInfo commandBufferAllocateInfo;
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.pNext = nullptr;
    commandBufferAllocateInfo.commandPool = _device->_commandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = static_cast<uint32_t>(_swapchain->_swapchain_data.image_set.get_image_count());
    
    //todo: remove "new"
    _command_buffers = new VkCommandBuffer[_swapchain->_swapchain_data.image_set.get_image_count()];
    VkResult result = vkAllocateCommandBuffers(_device->_logical_device, &commandBufferAllocateInfo, _command_buffers);
   ASSERT_VULKAN(result);
}




void renderer::create_semaphores()
{
    VkSemaphoreCreateInfo semaphoreCreateInfo;
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphoreCreateInfo.pNext = nullptr;
    semaphoreCreateInfo.flags = 0;
    
    VkResult result = vkCreateSemaphore(_device->_logical_device, &semaphoreCreateInfo, nullptr, &_semaphore_image_available);
    ASSERT_VULKAN(result);
    result = vkCreateSemaphore(_device->_logical_device, &semaphoreCreateInfo, nullptr, &_semaphore_rendering_done);
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
    _swapchain->recreate_swapchain(_render_pass );
    
    create_command_buffer();
    record_command_buffers();

}

void renderer::record_command_buffers()
{
    VkCommandBufferBeginInfo commandBufferBeginInfo;
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    commandBufferBeginInfo.pNext = nullptr;
    commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    commandBufferBeginInfo.pInheritanceInfo = nullptr;
    
    
    
    for (size_t i = 0; i < _swapchain->_swapchain_data.image_set.get_image_count(); i++)
    {
        VkResult result = vkBeginCommandBuffer(_command_buffers[i], &commandBufferBeginInfo);
        ASSERT_VULKAN(result);
        
        VkRenderPassBeginInfo renderPassBeginInfo;
        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.pNext = nullptr;
        renderPassBeginInfo.renderPass = _render_pass;
        renderPassBeginInfo.framebuffer = _swapchain->_swapchain_data.swapchain_frame_buffers[i];//framebuffers[i];
        renderPassBeginInfo.renderArea.offset = { 0, 0 };
        renderPassBeginInfo.renderArea.extent = { _swapchain->_swapchain_data.swapchain_extent.width, _swapchain->_swapchain_data.swapchain_extent.height };
        VkClearValue clearValue = {0.0f, 0.0f, 0.0f, 1.0f};
        VkClearValue depthClearValue = {1.0f, 0.0f};
        
        std::array<VkClearValue,2> clearValues;
        clearValues[0] = clearValue;
        clearValues[1] = depthClearValue;
        
        renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassBeginInfo.pClearValues = clearValues.data();
        
        
        vkCmdBeginRenderPass(_command_buffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
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

void renderer::destroy()
{
    vkDestroySemaphore(_device->_logical_device, _semaphore_image_available, nullptr);
    vkDestroySemaphore(_device->_logical_device, _semaphore_rendering_done, nullptr);
    _semaphore_image_available = VK_NULL_HANDLE;
    _semaphore_rendering_done = VK_NULL_HANDLE;
    
    vkFreeCommandBuffers(_device->_logical_device, _device->_commandPool,
                         static_cast<uint32_t>(_swapchain->_swapchain_data.image_set.get_image_count()), _command_buffers);
    delete[] _command_buffers;
    
    for (size_t i = 0; i < _swapchain->_swapchain_data.swapchain_frame_buffers.size(); i++)
    {
        vkDestroyFence(_device->_logical_device, _inflight_fences[i], nullptr);
    }
    
    _pipeline.destroy();
    vkDestroyRenderPass(_device->_logical_device, _render_pass, nullptr);
    _render_pass = VK_NULL_HANDLE;
}

renderer::~renderer()
{
}

void renderer::create_pipeline()
{
    _pipeline.create(_render_pass, _swapchain->_swapchain_data.swapchain_extent.width, _swapchain->_swapchain_data.swapchain_extent.height);
}

void renderer::init()
{
    create_render_pass();
    _material->create_descriptor_set_layout();
    _swapchain->recreate_swapchain(_render_pass);
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
    static uint32_t imageIndex = 0;
    vkWaitForFences(_device->_logical_device, 1, &_inflight_fences[imageIndex], VK_TRUE, std::numeric_limits<uint64_t>::max());
    vkResetFences(_device->_logical_device, 1, &_inflight_fences[imageIndex]);
    
    vkAcquireNextImageKHR(_device->_logical_device, _swapchain->_swapchain_data.swapchain, std::numeric_limits<uint64_t>::max(), _semaphore_image_available, VK_NULL_HANDLE, &imageIndex);
    
    VkSubmitInfo submitInfo;
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = nullptr;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &_semaphore_image_available;
    VkPipelineStageFlags waitStageMask[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.pWaitDstStageMask = waitStageMask;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &(_command_buffers[imageIndex]);
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &_semaphore_rendering_done;
    
    VkResult result = vkQueueSubmit(_device->_graphics_queue, 1, &submitInfo, _inflight_fences[imageIndex]);
    ASSERT_VULKAN(result);
    
    VkPresentInfoKHR presentInfo;
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = nullptr;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &_semaphore_rendering_done;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &_swapchain->_swapchain_data.swapchain;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;
    result = vkQueuePresentKHR(_device->_presentQueue, &presentInfo);
    
    ASSERT_VULKAN(result);
}

