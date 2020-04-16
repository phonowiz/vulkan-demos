//
//  command_buffer.hpp
//  vulkan-demos
//
//  Created by Rafael Sabino on 4/4/20.
//  Copyright © 2020 Rafael Sabino. All rights reserved.
//

#pragma once

#include "glfw_swapchain.h"
#include "device.h"
#include "EASTL/fixed_vector.h"
#include "EASTL/array.h"

namespace vk
{
    class command_recorder : public object
    {
    public:
        
        enum class command_type
        {
            GRAPHICS,
            COMPUTE
        };
        
        
        command_recorder(device* dev, glfw_swapchain& swapchain):
        _device(dev),
        _swapchain(swapchain)
        {
            VkCommandBufferAllocateInfo command_buffer_allocate_info {};
            command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            command_buffer_allocate_info.pNext = nullptr;
            command_buffer_allocate_info.commandPool = _device->_present_command_pool;
            command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            command_buffer_allocate_info.commandBufferCount = glfw_swapchain::NUM_SWAPCHAIN_IMAGES;
            
            VkResult result = vkAllocateCommandBuffers(_device->_logical_device, &command_buffer_allocate_info, _graphics_buffer.data());
            ASSERT_VULKAN(result);
            
            create_sync_objects();
        }
        
        void set_device( device* dev);
        void set_name(const char* name){ _name = name;};
        
        VkCommandBuffer& get_raw_graphics_command( uint32_t image_id)
        {
            return _graphics_buffer[image_id];
        };
        
        void begin_command_recording(uint32_t swapchain_image_id)
        {
            VkCommandBufferBeginInfo command_buffer_begin_info {};
            command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            command_buffer_begin_info.pNext = nullptr;
            command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
            command_buffer_begin_info.pInheritanceInfo = nullptr;
            
            VkResult result = vkBeginCommandBuffer(_graphics_buffer[swapchain_image_id], &command_buffer_begin_info);
            ASSERT_VULKAN(result);
        }
    
        VkCommandBuffer& get_raw_compute_command( uint32_t image_id )
        {
            assert( _device->_queue_family_indices.graphics_family.value() ==
                   _device->_queue_family_indices.compute_family.value());
            
            return _graphics_buffer[image_id];
            
        }
        
    
        void end_command_recording(uint32_t image_id)
        {
            vkEndCommandBuffer(_graphics_buffer[image_id]);
        }
        
        void submit_graphics_commands( uint32_t image_id )
        {
            uint32_t acquired_image = 0;
            vkAcquireNextImageKHR(_device->_logical_device, _swapchain.get_vk_swapchain(),
            std::numeric_limits<uint64_t>::max(),
            _acquire_semaphores[image_id], VK_NULL_HANDLE, &acquired_image);
            
            assert(image_id == acquired_image);
            
            VkResult result = {};
            VkSubmitInfo submit_info = {};
            submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submit_info.pNext = nullptr;
            submit_info.waitSemaphoreCount = 1;
            submit_info.pWaitSemaphores = &_acquire_semaphores[acquired_image];
            
            VkPipelineStageFlags wait_stage_mask[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
            
            submit_info.pWaitDstStageMask = wait_stage_mask;
            submit_info.commandBufferCount = 1;
            submit_info.pCommandBuffers = &_graphics_buffer[acquired_image];
            submit_info.signalSemaphoreCount = 1;
            submit_info.pSignalSemaphores = &_semaphores[acquired_image];
            
            vkResetFences(_device->_logical_device, 1, &_fences[image_id]);
            result = vkQueueSubmit(_device->_graphics_queue, 1, &submit_info, _fences[acquired_image]);
            
            ASSERT_VULKAN(result);
            //present the scene to viewer
            VkPresentInfoKHR present_info {};
            present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            present_info.pNext = nullptr;
            present_info.waitSemaphoreCount = 1;
            present_info.pWaitSemaphores =&_semaphores[acquired_image];
            present_info.swapchainCount = 1;
            present_info.pSwapchains = &(_swapchain.get_vk_swapchain());
            present_info.pImageIndices = &acquired_image;
            present_info.pResults = nullptr;
            result = vkQueuePresentKHR(_device->_present_queue, &present_info);

            ASSERT_VULKAN(result);
        }
        
        void submit_compute_commands ( uint32_t image_id)
        {
            
            assert(_device->_compute_queue == _device->_graphics_queue && "compute queue and graphics queue must be the same"
                                                                            "for this function to work");
            submit_graphics_commands(image_id);
        }
        
        void reset( uint32_t image_id )
        {
            vkWaitForFences(_device->_logical_device, 1, &_fences[image_id], VK_TRUE, std::numeric_limits<uint64_t>::max());
            
            static const VkCommandBufferResetFlags flags = 0;
            vkResetCommandBuffer( _graphics_buffer[image_id], flags );
        }
        
        void destroy() override
        {
            vkFreeCommandBuffers(_device->_logical_device, _device->_graphics_command_pool,
                                 glfw_swapchain::NUM_SWAPCHAIN_IMAGES, _graphics_buffer.data()) ;
            
            for( int  i = 0; i < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++i)
            {
                vkDestroyFence(_device->_logical_device, _fences[i] , nullptr);
                _fences[i] = VK_NULL_HANDLE;
                vkDestroySemaphore(_device->_logical_device, _semaphores[i], nullptr);
                _semaphores[i] = VK_NULL_HANDLE;
                vkDestroySemaphore(_device->_logical_device, _acquire_semaphores[i],nullptr);
                _acquire_semaphores[i] = VK_NULL_HANDLE;
            }
        };
    private:
        
        void create_sync_objects()
        {
            for( int i = 0; i < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++i)
            {
                VkSemaphoreCreateInfo semaphore_create_info {};
                semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
                semaphore_create_info.pNext = nullptr;
                semaphore_create_info.flags = 0;
                
                
                VkResult result = vkCreateSemaphore(_device->_logical_device, &semaphore_create_info, nullptr, &_semaphores[i]);
                ASSERT_VULKAN(result);
                result = vkCreateSemaphore(_device->_logical_device, &semaphore_create_info, nullptr, &_acquire_semaphores[i]);
                ASSERT_VULKAN(result);
                
                create_fence(_fences[i]);
            }
        }
        
        void create_fence(VkFence& fence)
        {
            VkFenceCreateInfo fenceInfo = {};
            fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
            
            VkResult result = vkCreateFence(_device->_logical_device, &fenceInfo, nullptr, &fence);
            ASSERT_VULKAN(result);
        }
        
    private:
        
        device* _device = nullptr;
        const char* _name = nullptr;
        glfw_swapchain& _swapchain;
        
        eastl::array<VkCommandBuffer, glfw_swapchain::NUM_SWAPCHAIN_IMAGES> _graphics_buffer {};
        eastl::array<VkSemaphore, glfw_swapchain::NUM_SWAPCHAIN_IMAGES> _semaphores {};
        eastl::array<VkSemaphore, glfw_swapchain::NUM_SWAPCHAIN_IMAGES> _acquire_semaphores{};
        eastl::array<VkFence, glfw_swapchain::NUM_SWAPCHAIN_IMAGES>  _fences {};

    };
}
