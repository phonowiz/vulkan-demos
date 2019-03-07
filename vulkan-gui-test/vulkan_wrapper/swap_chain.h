//
//  swap_chain.hpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 2/7/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#pragma once

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include <array>

#include "physical_device.h"
#include <vector>
#include "depth_image.h"
#include "texture.h"
#include "swapchain_image_set.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>


struct GLFWwindow;


namespace vk
{
    class SwapChain : public object
    {
        
    public:
        
        static const int MAX_SWAPCHAIN_IMAGES = 5;
        struct SwapChainData
        {
            VkSwapchainKHR          swapChain = VK_NULL_HANDLE;
            //std::vector<VkImage>    swapChainImages;
            SwapchainImageSet       imageSet;
            //VkFormat                swapChainImageFormat;
            VkExtent2D              swapChainExtent;
            
            //std::vector<VkImageView>    swapChainImageViews;
            std::vector<VkFramebuffer>  swapChainFramebuffers;
            std::array<Texture, MAX_SWAPCHAIN_IMAGES>        presentTextures;
        };
        
        PhysicalDevice* _physicalDevice = nullptr;
        
        SwapChain(PhysicalDevice* physicalDevice, GLFWwindow* window);
        
        void setPhysicalDevice(PhysicalDevice* physicalDevice){ _physicalDevice = physicalDevice; }
        void getSwapchainSupportDetails(PhysicalDevice::SwapChainSupportDetails& details);
        VkSurfaceFormatKHR getSurfaceFormat();
        void printStats();
        
        
        SwapChainData _swapChainData;
        GLFWwindow*   _window = nullptr;
        VkSurfaceKHR  _surface = VK_NULL_HANDLE;
        
        DepthImage          _depthImage;
        
        VkSurfaceFormatKHR  chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
        VkPresentModeKHR    chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
        VkExtent2D          chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow& window);
        void                createSwapChain( );
        void                querySwapChainSupport( PhysicalDevice::SwapChainSupportDetails& );
        void                createSurface( );
        void                createImageViews();
        void                destroySwapChain();
        void                recreateSwapChain(VkRenderPass renderPass);
        void                createDepthImage( );
        void                createFrameBuffers(VkRenderPass renderPass);
        VkAttachmentDescription                 getDepthAttachment();
        VkPipelineDepthStencilStateCreateInfo   getDepthStencilStateCreateInfoOpaque();
        
        virtual void  destroy() override;
        ~SwapChain();
        
    private:
    };
}

