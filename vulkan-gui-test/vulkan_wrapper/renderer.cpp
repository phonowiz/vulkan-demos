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
#include "vulkan_wrapper/shapes/obj_shape.h"
#include "shader.h"
#include "device.h"
#include <chrono>
#include <algorithm>
#include <stdio.h>

using namespace vk;


renderer::renderer(device* device, GLFWwindow* window, swapchain* swapchain, visual_mat_shared_ptr material):
_depth_image(device,swapchain->_swapchain_data.swapchain_extent.width, swapchain->_swapchain_data.swapchain_extent.height, false),
_pipeline(device, material)
{
    _device = device;
    _window = window;
    _swapchain = swapchain;
    _material = material;
}

void renderer::create_render_pass()
{
    //note: color attachments go first, then depth attachment.
    //note: VkAttachmentDescription  are  used to create the renderpass, while the
    //VkAttachmentReference are used to create the subpass itself
    uint32_t num_attachments = 0;
    std::array<VkAttachmentDescription, MAX_ATTACHMENTS> attachment_descriptions {};
    attachment_descriptions[num_attachments].flags = 0;
    attachment_descriptions[num_attachments].format = _swapchain->get_surface_format().format;
    attachment_descriptions[num_attachments].samples = VK_SAMPLE_COUNT_1_BIT;
    attachment_descriptions[num_attachments].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment_descriptions[num_attachments].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment_descriptions[num_attachments].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment_descriptions[num_attachments].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment_descriptions[num_attachments].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment_descriptions[num_attachments].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    ++num_attachments;
    
    //note: the last attachment will always be the depth
    attachment_descriptions[num_attachments] = _depth_image.get_depth_attachment();
    ++num_attachments;
    
    //an excellent explanation of what the heck are these attachment references:
    //https://stackoverflow.com/questions/49652207/what-is-the-purpose-of-vkattachmentreference
    
    std::array<VkAttachmentReference, 2> attachment_references {};
    
    attachment_references[0].attachment = 0;
    //this is the layout the attahment will be used during the subpass.  The driver decides if there should be a
    //transition or not given the 'initialLayout' specified in the attachment description
    attachment_references[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL ;
    
    VkAttachmentReference depth_attachment_reference;
    depth_attachment_reference.attachment = num_attachments-1;
    depth_attachment_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    
    assert(num_attachments >= 2);
    
    //here is article about subpasses and input attachments and how they are all tied togethere
    //https://www.saschawillems.de/blog/2018/07/19/vulkan-input-attachments-and-sub-passes/
    VkSubpassDescription subpass_description {};
    subpass_description.flags = 0;
    subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_description.inputAttachmentCount = 0;
    subpass_description.pInputAttachments = nullptr;
    subpass_description.colorAttachmentCount = 1;
    subpass_description.pColorAttachments = attachment_references.data();
    subpass_description.pResolveAttachments = nullptr;
    subpass_description.pDepthStencilAttachment = &depth_attachment_reference;
    subpass_description.preserveAttachmentCount = 0;
    subpass_description.pPreserveAttachments = nullptr;

    //Use subpass dependencies for attachment layput transitions.
    //Based off of this example: https://github.com/SaschaWillems/Vulkan/blob/master/examples/deferred/deferred.cpp
    std::array<VkSubpassDependency, 2> subpass_dependencies;

    //note: for a great explanation of VK_SUBPASS_EXTERNAL:
    //https://stackoverflow.com/questions/53984863/what-exactly-is-vk-subpass-external?rq=1
    subpass_dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    subpass_dependencies[0].dstSubpass = 0;
    subpass_dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_dependencies[0].srcAccessMask = 0;
    subpass_dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subpass_dependencies[0].dependencyFlags = 0;
    
    
    subpass_dependencies[1].srcSubpass = 0;
    subpass_dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    subpass_dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    subpass_dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subpass_dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    subpass_dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    
    //note: because we state the images initial layout upon entering the renderpass and the layouts the subpasses
    //need for them to do their work, transitions are implicit and will happen automatically as the renderpass excecutes.

    VkRenderPassCreateInfo render_pass_create_info;
    render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_create_info.pNext = nullptr;
    render_pass_create_info.flags = 0;
    render_pass_create_info.attachmentCount = num_attachments;
    render_pass_create_info.pAttachments = attachment_descriptions.data();
    render_pass_create_info.subpassCount = 1;
    render_pass_create_info.pSubpasses = &subpass_description;
    render_pass_create_info.dependencyCount = 2;
    render_pass_create_info.pDependencies = subpass_dependencies.data();
    
    VkResult result = vkCreateRenderPass(_device->_logical_device, &render_pass_create_info, nullptr, &_render_pass);

    ASSERT_VULKAN(result);
}


void renderer::create_command_buffer(VkCommandBuffer** command_buffers, VkCommandPool command_pool)
{
    VkCommandBufferAllocateInfo command_buffer_allocate_info;
    command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.pNext = nullptr;
    command_buffer_allocate_info.commandPool = command_pool;
    command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_allocate_info.commandBufferCount = static_cast<uint32_t>(_swapchain->_swapchain_data.image_set.get_image_count());

    //todo: remove "new"
    delete[] *command_buffers;
    *command_buffers = new VkCommandBuffer[_swapchain->_swapchain_data.image_set.get_image_count()];
    
    

    VkResult result = vkAllocateCommandBuffers(_device->_logical_device, &command_buffer_allocate_info, *command_buffers);
    ASSERT_VULKAN(result);
}



void renderer::create_semaphore(VkSemaphore& semaphore)
{
    VkSemaphoreCreateInfo semaphore_create_info;
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
    
    for(int i = 0; i < _swapchain->_swapchain_data.image_set.get_image_count(); ++i)
    {
        create_fence(_inflight_fences[i]);
    }
}

void renderer::recreate_renderer()
{

    _device->wait_for_all_operations_to_finish();

    vkDestroyRenderPass(_device->_logical_device, _render_pass, nullptr);
    _render_pass = VK_NULL_HANDLE;
    create_render_pass();
    destroy_framebuffers();
    _swapchain->recreate_swapchain( );
    create_frame_buffers();
    
    create_command_buffer(&_command_buffers, _device->_present_command_pool);
    record_command_buffers(_shapes.data(), _shapes.size());

}

void renderer::record_command_buffers(obj_shape** shapes, size_t number_of_shapes)
{
    VkCommandBufferBeginInfo command_buffer_begin_info {};
    command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_begin_info.pNext = nullptr;
    command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    command_buffer_begin_info.pInheritanceInfo = nullptr;
    
    
    
    for (size_t i = 0; i < _swapchain->_swapchain_data.image_set.get_image_count(); i++)
    {
        VkResult result = vkBeginCommandBuffer(_command_buffers[i], &command_buffer_begin_info);
        ASSERT_VULKAN(result);
        
        VkRenderPassBeginInfo render_pass_create_info = {};
        render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_create_info.pNext = nullptr;
        render_pass_create_info.renderPass = _render_pass;
        render_pass_create_info.framebuffer = _swapchain_frame_buffers[i];
        render_pass_create_info.renderArea.offset = { 0, 0 };
        render_pass_create_info.renderArea.extent = { _swapchain->_swapchain_data.swapchain_extent.width,
                                                        _swapchain->_swapchain_data.swapchain_extent.height };
        VkClearValue clear_value = {0.0f, 0.0f, 0.0f, 1.0f};
        VkClearValue depth_clear_value = {1.0f, 0.0f};
        
        std::array<VkClearValue,2> clear_values;
        clear_values[0] = clear_value;
        clear_values[1] = depth_clear_value;
        
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
        scissor.extent = { _swapchain->_swapchain_data.swapchain_extent.width,
                                _swapchain->_swapchain_data.swapchain_extent.height};
        vkCmdSetScissor(_command_buffers[i], 0, 1, &scissor);

        
        for( size_t j = 0; j < number_of_shapes; ++j)
        {
            shapes[j]->draw(_command_buffers[i], _pipeline);
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
    _device->wait_for_all_operations_to_finish();
    vkDestroySemaphore(_device->_logical_device, _semaphore_image_available, nullptr);
    vkDestroySemaphore(_device->_logical_device, _semaphore_rendering_done, nullptr);
    _semaphore_image_available = VK_NULL_HANDLE;
    _semaphore_rendering_done = VK_NULL_HANDLE;
    
    vkFreeCommandBuffers(_device->_logical_device, _device->_graphics_command_pool,
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
    _depth_image.init();

    //TODO: Get rid of the vector class
    _swapchain_frame_buffers.resize(_swapchain->_swapchain_data.image_set.get_image_count());
    
    for (size_t i = 0; i < _swapchain_frame_buffers.size(); i++)
    {
        VkImageView depth_image_view = _depth_image.get_image_view();
        std::array<VkImageView, MAX_ATTACHMENTS> attachment_views {};
        
        assert(depth_image_view != VK_NULL_HANDLE);
        
        uint32_t num_attachments = 0;
        attachment_views[num_attachments] = _swapchain->_swapchain_data.image_set.get_image_views()[i];
        num_attachments++;

        attachment_views[num_attachments] = depth_image_view;
        num_attachments++;
        
        VkFramebufferCreateInfo framebuffer_create_info {};
        framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_create_info.pNext = nullptr;
        framebuffer_create_info.flags = 0;
        framebuffer_create_info.renderPass = _render_pass;
        framebuffer_create_info.attachmentCount = num_attachments;
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
    assert(_shapes.size() != 0 && "add meshes to render before calling init");
    create_render_pass();
    //_swapchain->recreate_swapchain();
    
    create_frame_buffers();
    create_semaphores_and_fences();
    create_command_buffer(&_command_buffers, _device->_present_command_pool);
    
    //we only support 1 mesh at the moment
    //assert(_shapes.size() == 1);
    assert(_shapes.size() != 0);
    
}

void renderer::perform_final_drawing_setup()
{
    _material->commit_parameters_to_gpu();

    if(!_pipeline_created)
    {
        //note: we create the pipeline here to give the client a chance to set the material input arguments.
        //the pipeline needs this information to be created properly.
        create_pipeline();
        record_command_buffers(_shapes.data(), _shapes.size());
        _pipeline_created = true;
    }
}

void renderer::draw(camera& camera)
{
    
    assert(_material != nullptr);
    perform_final_drawing_setup();
    
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
    result = vkQueuePresentKHR(_device->_present_queue, &present_info);
    
    ASSERT_VULKAN(result);
}

renderer::~renderer()
{
}
