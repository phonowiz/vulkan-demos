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
#include "physical_device.h"
#include "swap_chain.h"
#include "depth_image.h"
#include "vertex.h"
#include "vulkan_wrapper/mesh.h"
#include "shader.h"
#include "physical_device.h"
#include <chrono>
#include <algorithm>


using namespace vk;


Renderer::Renderer(PhysicalDevice* physicalDevice, GLFWwindow* window, SwapChain* swapChain, MaterialSharedPtr material):
_pipeline(physicalDevice, material)
{
    _physicalDevice = physicalDevice;
    _window = window;
    _swapChain = swapChain;
    _material = material;
}

void Renderer::createRenderPass()
{
    
    VkAttachmentDescription attachmentDescription;
    attachmentDescription.flags = 0;
    attachmentDescription.format = _swapChain->getSurfaceFormat().format;
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
    
    VkAttachmentDescription depthAttachment = _swapChain->getDepthAttachment();
    
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
    
    VkResult result = vkCreateRenderPass(_physicalDevice->_device, &renderPassCreateInfo, nullptr, &_renderPass);

    ASSERT_VULKAN(result);
}


void Renderer::createCommandBuffers()
{
    VkCommandBufferAllocateInfo commandBufferAllocateInfo;
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.pNext = nullptr;
    commandBufferAllocateInfo.commandPool = _physicalDevice->_commandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = static_cast<uint32_t>(_swapChain->_swapChainData.imageSet.getImageCount());
    
    //todo: remove "new"
    _commandBuffers = new VkCommandBuffer[_swapChain->_swapChainData.imageSet.getImageCount()];
    VkResult result = vkAllocateCommandBuffers(_physicalDevice->_device, &commandBufferAllocateInfo, _commandBuffers);
   ASSERT_VULKAN(result);
}




void Renderer::createSemaphores()
{
    VkSemaphoreCreateInfo semaphoreCreateInfo;
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphoreCreateInfo.pNext = nullptr;
    semaphoreCreateInfo.flags = 0;
    
    VkResult result = vkCreateSemaphore(_physicalDevice->_device, &semaphoreCreateInfo, nullptr, &_semaphoreImageAvailable);
    ASSERT_VULKAN(result);
    result = vkCreateSemaphore(_physicalDevice->_device, &semaphoreCreateInfo, nullptr, &_semaphoreRenderingDone);
    ASSERT_VULKAN(result);
    
    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    
    
    for(int i = 0; i < _swapChain->_swapChainData.imageSet.getImageCount(); ++i)
    {
        result = vkCreateFence(_physicalDevice->_device, &fenceInfo, nullptr, &_inFlightFences[i]);
        ASSERT_VULKAN(result);
    }
}

void Renderer::recreateRenderer()
{

    _physicalDevice->waitForllOperationsToFinish();

    vkDestroyRenderPass(_physicalDevice->_device, _renderPass, nullptr);

    createRenderPass();
    _swapChain->recreateSwapChain(_renderPass );
    
    createCommandBuffers();
    recordCommandBuffers();

}

void Renderer::recordCommandBuffers()
{
    VkCommandBufferBeginInfo commandBufferBeginInfo;
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    commandBufferBeginInfo.pNext = nullptr;
    commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    commandBufferBeginInfo.pInheritanceInfo = nullptr;
    
    
    
    for (size_t i = 0; i < _swapChain->_swapChainData.imageSet.getImageCount(); i++)
    {
        VkResult result = vkBeginCommandBuffer(_commandBuffers[i], &commandBufferBeginInfo);
        ASSERT_VULKAN(result);
        
        VkRenderPassBeginInfo renderPassBeginInfo;
        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.pNext = nullptr;
        renderPassBeginInfo.renderPass = _renderPass;
        renderPassBeginInfo.framebuffer = _swapChain->_swapChainData.swapChainFramebuffers[i];//framebuffers[i];
        renderPassBeginInfo.renderArea.offset = { 0, 0 };
        renderPassBeginInfo.renderArea.extent = { _swapChain->_swapChainData.swapChainExtent.width, _swapChain->_swapChainData.swapChainExtent.height };
        VkClearValue clearValue = {0.0f, 0.0f, 0.0f, 1.0f};
        VkClearValue depthClearValue = {1.0f, 0.0f};
        
        std::array<VkClearValue,2> clearValues;
        clearValues[0] = clearValue;
        clearValues[1] = depthClearValue;
        
        renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassBeginInfo.pClearValues = clearValues.data();
        
        
        vkCmdBeginRenderPass(_commandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline._pipeline);
        
        VkViewport viewport;
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = _swapChain->_swapChainData.swapChainExtent.width;
        viewport.height = _swapChain->_swapChainData.swapChainExtent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(_commandBuffers[i], 0, 1, &viewport);
        
        VkRect2D scissor;
        scissor.offset = { 0, 0};
        scissor.extent = { _swapChain->_swapChainData.swapChainExtent.width, _swapChain->_swapChainData.swapChainExtent.height};
        vkCmdSetScissor(_commandBuffers[i], 0, 1, &scissor);

        
        for( vk::Mesh* pMesh : _meshes)
        {
            pMesh->draw(_commandBuffers[i], _pipeline);
        }
        
        vkCmdEndRenderPass(_commandBuffers[i]);
        
        result = vkEndCommandBuffer(_commandBuffers[i]);
        ASSERT_VULKAN(result);
    }
}

void Renderer::destroy()
{
    vkDestroySemaphore(_physicalDevice->_device, _semaphoreImageAvailable, nullptr);
    vkDestroySemaphore(_physicalDevice->_device, _semaphoreRenderingDone, nullptr);
    _semaphoreImageAvailable = VK_NULL_HANDLE;
    _semaphoreRenderingDone = VK_NULL_HANDLE;
    
    vkFreeCommandBuffers(_physicalDevice->_device, _physicalDevice->_commandPool,
                         static_cast<uint32_t>(_swapChain->_swapChainData.imageSet.getImageCount()), _commandBuffers);
    delete[] _commandBuffers;
    
    for (size_t i = 0; i < _swapChain->_swapChainData.swapChainFramebuffers.size(); i++)
    {
        vkDestroyFence(_physicalDevice->_device, _inFlightFences[i], nullptr);
    }
    
    _pipeline.destroy();
    vkDestroyRenderPass(_physicalDevice->_device, _renderPass, nullptr);
    _renderPass = VK_NULL_HANDLE;
}

Renderer::~Renderer()
{
}

void Renderer::createPipeline()
{
    _pipeline.create(_renderPass, _swapChain->_swapChainData.swapChainExtent.width, _swapChain->_swapChainData.swapChainExtent.height);
}

void Renderer::init()
{
    createRenderPass();
    _material->createDescriptorSetLayout();
    _swapChain->recreateSwapChain(_renderPass);
    createPipeline();
    createSemaphores();

    createCommandBuffers();


    //we only support 1 mesh at the moment
    assert(_meshes.size() == 1);
    assert(_meshes.size() != 0);
    
    for(int i = 0; i < _meshes.size(); ++i)
    {
        _meshes[i]->allocateGPUMemory();
    }
    createCommandBuffers();
    recordCommandBuffers();
    
}

void Renderer::draw()
{
    static uint32_t imageIndex = 0;
    vkWaitForFences(_physicalDevice->_device, 1, &_inFlightFences[imageIndex], VK_TRUE, std::numeric_limits<uint64_t>::max());
    vkResetFences(_physicalDevice->_device, 1, &_inFlightFences[imageIndex]);
    
    vkAcquireNextImageKHR(_physicalDevice->_device, _swapChain->_swapChainData.swapChain, std::numeric_limits<uint64_t>::max(), _semaphoreImageAvailable, VK_NULL_HANDLE, &imageIndex);
    
    VkSubmitInfo submitInfo;
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = nullptr;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &_semaphoreImageAvailable;
    VkPipelineStageFlags waitStageMask[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.pWaitDstStageMask = waitStageMask;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &(_commandBuffers[imageIndex]);
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &_semaphoreRenderingDone;
    
    VkResult result = vkQueueSubmit(_physicalDevice->_graphicsQueue, 1, &submitInfo, _inFlightFences[imageIndex]);
    ASSERT_VULKAN(result);
    
    VkPresentInfoKHR presentInfo;
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = nullptr;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &_semaphoreRenderingDone;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &_swapChain->_swapChainData.swapChain;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;
    result = vkQueuePresentKHR(_physicalDevice->_presentQueue, &presentInfo);
    
    ASSERT_VULKAN(result);
}

