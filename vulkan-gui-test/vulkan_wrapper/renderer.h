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

#include <array>
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
    
    class renderer : public object
    {
    public:
        
        renderer(device* physicalDevice, GLFWwindow* window, swapchain* swapChain, material_shared_ptr material);
        
        void create_render_pass();
        void create_command_buffer();
        void create_semaphores();
        void recreate_renderer();
        void record_command_buffers();
        void create_pipeline();
        void add_mesh(mesh* pMesh){ _meshes.push_back(pMesh); };
        void clear_meshes(mesh* pMesh){ _meshes.clear();}
        void draw();
        pipeline& get_pipeline() { return _pipeline;}
        
        void init();

        device*     _device = nullptr;
        GLFWwindow*         _window = nullptr;
        VkRenderPass        _render_pass = VK_NULL_HANDLE;

        VkSemaphore _semaphore_image_available = VK_NULL_HANDLE;
        VkSemaphore _semaphore_rendering_done = VK_NULL_HANDLE;
        swapchain*  _swapchain = nullptr;

        std::array<VkFence, 20> _inflight_fences;
        VkCommandBuffer* _command_buffers = nullptr;
        pipeline _pipeline;

        material_shared_ptr _material;

        std::vector<mesh*> _meshes;
        virtual void destroy() override;
        ~renderer();
        
    private:
    };
}
