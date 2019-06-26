//
//  renderer.hpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 2/7/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#pragma once


#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>
#include "material.h"
#include "depth_image.h"
#include "pipeline.h"



namespace vk
{
    class device;
    class swapchain;
    class depth_image;
    class mesh;
    class texture_2d;
    
    class renderer : public object
    {
        
    public:
        enum class subpass_layout
        {
            RENDER_TARGET_LAYOUT = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            READ_ONLY_LAYOUT = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            DEPTH_READ_ONLY_LAYOUT = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
            LAYOUT_UNDEFINED = VK_IMAGE_LAYOUT_UNDEFINED
        };

        
    public:
        
        renderer(device* physicalDevice, GLFWwindow* window, swapchain* swapChain, material_shared_ptr material);
        

        void add_mesh(mesh* pMesh){ _meshes.push_back(pMesh); };
        void clear_meshes(mesh* pMesh){ _meshes.clear();}
        virtual void draw();
        pipeline& get_pipeline() { return _pipeline;}
        
        void init();
        void recreate_renderer();
        virtual vk::material_shared_ptr & get_material(){ return _material; }
        
        virtual void destroy() override;
        ~renderer();
        
        
    protected:
    
        virtual void create_frame_buffers();
        virtual void create_render_pass();
        virtual void create_command_buffer(VkCommandBuffer** command_buffers);
        virtual void create_semaphores_and_fences();
        
        void create_semaphore(VkSemaphore& semaphore);
        void create_fence(VkFence& fence);
        
        virtual void record_command_buffers(mesh* meshes, size_t number_of_meshes);
        virtual void create_pipeline();
        virtual void destroy_framebuffers();
        virtual void perform_final_drawing_setup();
    
    protected:
        
        device*             _device = nullptr;
        GLFWwindow*         _window = nullptr;
        VkRenderPass        _render_pass = VK_NULL_HANDLE;
        
        VkSemaphore _semaphore_image_available = VK_NULL_HANDLE;
        VkSemaphore _semaphore_rendering_done = VK_NULL_HANDLE;
        swapchain*  _swapchain = nullptr;
        
        std::array<VkFence, 20> _inflight_fences {};

        
        //todo: find out what is the limit of attachments
        static const uint32_t MAX_ATTACHMENTS = 10;
        
        VkCommandBuffer* _command_buffers = nullptr;
        pipeline _pipeline;
        
        material_shared_ptr         _material;
        std::vector<VkFramebuffer>  _swapchain_frame_buffers;
        
        std::vector<mesh*> _meshes;
        
        depth_image _depth_image;

    };
}
