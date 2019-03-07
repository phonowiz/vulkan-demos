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

class Mesh;


namespace vk
{
    class PhysicalDevice;
    class SwapChain;
    class DepthImage;
    class Mesh;
    
    class Renderer : public object
    {
    public:
        
        Renderer(PhysicalDevice* physicalDevice, GLFWwindow* window, SwapChain* swapChain, MaterialSharedPtr material);
        
        void createRenderPass();
        void createCommandBuffers();
        void createSemaphores();
        void recreateRenderer();
        void recordCommandBuffers();
        void createPipeline();
        void addMesh(Mesh* pMesh){ _meshes.push_back(pMesh); };
        void clearMeshes(Mesh* pMesh){ _meshes.clear();}
        void clearMesh();
        void draw();
        
        void init();

        PhysicalDevice*     _physicalDevice = nullptr;
        GLFWwindow*         _window = nullptr;
        VkRenderPass        _renderPass = VK_NULL_HANDLE;

        VkSemaphore _semaphoreImageAvailable = VK_NULL_HANDLE;
        VkSemaphore _semaphoreRenderingDone = VK_NULL_HANDLE;
        SwapChain*  _swapChain = nullptr;

        std::array<VkFence, 20> _inFlightFences;
        VkCommandBuffer* _commandBuffers = nullptr;
        VkPipelineLayout _pipelineLayout;
        VkPipeline       _pipeline  = VK_NULL_HANDLE;

        MaterialSharedPtr _material;

        std::vector<Mesh*> _meshes;
        virtual void destroy() override;
        ~Renderer();
        
    private:
    };
}
