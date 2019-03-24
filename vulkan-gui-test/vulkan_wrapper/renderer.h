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
    
    class Renderer : public object
    {
    public:
        
        Renderer(device* physicalDevice, GLFWwindow* window, swapchain* swapChain, material_shared_ptr material);
        
        void createRenderPass();
        void createCommandBuffers();
        void createSemaphores();
        void recreateRenderer();
        void recordCommandBuffers();
        void createPipeline();
        void addMesh(mesh* pMesh){ _meshes.push_back(pMesh); };
        void clearMeshes(mesh* pMesh){ _meshes.clear();}
        void clearMesh();
        void draw();
        pipeline& getPipeline() { return _pipeline;}
        
        void init();

        device*     _physicalDevice = nullptr;
        GLFWwindow*         _window = nullptr;
        VkRenderPass        _renderPass = VK_NULL_HANDLE;

        VkSemaphore _semaphoreImageAvailable = VK_NULL_HANDLE;
        VkSemaphore _semaphoreRenderingDone = VK_NULL_HANDLE;
        swapchain*  _swapChain = nullptr;

        std::array<VkFence, 20> _inFlightFences;
        VkCommandBuffer* _commandBuffers = nullptr;
        pipeline _pipeline;

        material_shared_ptr _material;

        std::vector<mesh*> _meshes;
        virtual void destroy() override;
        ~Renderer();
        
    private:
    };
}
