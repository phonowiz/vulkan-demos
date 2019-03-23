// VulkanTutorial.cpp : Defines the entry point for the console application.
//

//#include "stdafx.h"
#define VK_USE_PLATFORM_MACOS_MVK
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_CTOR_INIT
#define GLM_FORCE_SILENT_WARNINGS
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>


#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

//#include "easy_image.h"
//#include "depth_image.h"
//#include "vertex.h"

//#include "create_swapchain.hpp"


#include <assert.h>
#include <vector>
#include <array>
#include <algorithm>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>

///an excellent summary of vulkan can be found here:
//https://renderdoc.org/vulkan-in-30-minutes.html


GLFWwindow *window = nullptr;
VkSurfaceKHR surface;

int width = 1024;
int height = 768;

void startGlfw() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_TRUE);
    
    window = glfwCreateWindow(width, height, "Vulkan Tutorial", nullptr, nullptr);
    //glfwSetWindowSizeCallback(window, onWindowResized);
}

//render target
std::chrono::time_point gameStartTime = std::chrono::high_resolution_clock::now();



void shutdownGlfw() {
    glfwDestroyWindow(window);
    
    glfwTerminate();
}


#include "vulkan_wrapper/physical_device.h"
#include "vulkan_wrapper/renderer.h"
#include "vulkan_wrapper/swap_chain.h"
#include "vulkan_wrapper/material_store.h"
#include "vulkan_wrapper/mesh.h"
#include "vulkan_wrapper/display_plane.h"


vk::MaterialSharedPtr standardMat;
vk::MaterialSharedPtr displayMat;



vk::Texture2D* texture = nullptr;

void updateMVP2()
{
    //todo: this should work for every mesh
    std::chrono::time_point frameTime = std::chrono::high_resolution_clock::now();
    float timeSinceStart = std::chrono::duration_cast<std::chrono::milliseconds>( frameTime - gameStartTime ).count()/1000.0f;
    
    glm::mat4 model = glm::rotate(glm::mat4(1.0f), timeSinceStart * glm::radians(30.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 view = glm::lookAt(glm::vec3(1.0f, 0.0f, -1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 projection = glm::perspective(glm::radians(60.0f), width/(float)height, 0.01f, 10.0f);
    
    /*
     GLM was originally designed for OpenGL, where the Y coordinate of the clip coordinates is inverted.
     The easiest way to compensate for that is to flip the sign on the scaling factor of the Y axis in the projection matrix.
     If you don't do this, then the image will be rendered upside down.
     */
    projection[1][1] *= -1.0f;
    
    glm::vec4 temp =(glm::rotate(glm::mat4(1.0f), timeSinceStart * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)) * glm::vec4(0.0f, 3.0f, 1.0f, 0.0f));

    vk::ShaderParameter::ShaderParamsGroup& vertexParams =   standardMat->getUniformParameters(vk::Material::ParameterStage::VERTEX, 0);
    
    vertexParams["model"] = model;
    vertexParams["view"] = view;
    vertexParams["projection"] = projection;
    vertexParams["lightPosition"] = temp;
    
    
    static bool initted = false;
    if(initted == false)
    {
        initted = true;
        standardMat->setImageSampler(texture, "tex", vk::Material::ParameterStage::FRAGMENT, 1);
    }
    
    standardMat->commitParametersToGPU();
}

void gameLoop2(vk::Renderer &renderer)
{
    int i = 0;
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        updateMVP2();
        renderer.draw();
        ++i;
    }
}


struct App
{
    vk::SwapChain* swapchain = nullptr;
    vk::PhysicalDevice* physical_device = nullptr;
    vk::Renderer* renderer = nullptr;
};


App app;

void onWindowResized2(GLFWwindow * window, int w, int h)
{
    if( w != 0 && h != 0)
    {
        
        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(app.physical_device->_physicalDevice, surface, &surfaceCapabilities);
        
        w = std::min(w, static_cast<int>(surfaceCapabilities.maxImageExtent.width));
        h = std::min(h, static_cast<int>(surfaceCapabilities.maxImageExtent.height));
        
        w = std::max(w, static_cast<int>(surfaceCapabilities.minImageExtent.width));
        h = std::max(h, static_cast<int>(surfaceCapabilities.minImageExtent.height));
        
        width = w;
        height = h;
        app.renderer->recreateRenderer();
        
    }
}



void updateWithOrtho()
{

    vk::ShaderParameter::ShaderParamsGroup& vertexParams = displayMat->getUniformParameters(vk::Material::ParameterStage::VERTEX, 0);
    vertexParams["width"] = width;
    vertexParams["height"] = height;
    
    
    displayMat->setImageSampler(texture, "tex", vk::Material::ParameterStage::FRAGMENT, 1);
    
    displayMat->commitParametersToGPU();
    
}

void gameLoop3(vk::Renderer &renderer)
{
    int i = 0;
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        //updateMVP2();
        updateWithOrtho();
        renderer.draw();
        ++i;
    }
}

int main()
{
    startGlfw();

    //vulkan render example
    glfwSetWindowSizeCallback(window, onWindowResized2);
    
    vk::PhysicalDevice device;
    
    //createSurface(device._instance, window, surface);
    VkResult res = glfwCreateWindowSurface(device._instance, window, nullptr, &surface);
    assert(res == VK_SUCCESS);
    
    device.createLogicalDevice(surface);
    
    vk::SwapChain swapChain(&device, window);
    vk::MaterialStore materialStore;
    
    materialStore.createStore(&device);
    
    standardMat = materialStore.GET_MAT<vk::Material>("standard");
    displayMat = materialStore.GET_MAT<vk::Material>("display");
    //vk::Texture texture(&device, "mario.png");
    
    vk::Mesh mesh( "dragon.obj", &device );
    
    vk::display_plane plane(&device);
    plane.create();
    
    vk::Texture2D mario(&device, "mario.png");
    texture = &mario;
    //updateMVP2();
    
    updateWithOrtho();
    vk::Renderer renderer(&device,window, &swapChain, displayMat);
    
    renderer.addMesh(&plane);
    renderer.init();
    
    app.physical_device = &device;
    app.swapchain = &swapChain;
    app.renderer = &renderer;
    
    //gameLoop2(renderer);
    gameLoop3(renderer);
    
    device.waitForllOperationsToFinish();
    swapChain.destroy();
    materialStore.destroy();
    mesh.destroy();
    plane.destroy();
    renderer.destroy();
    device.destroy();
    
    shutdownGlfw();
    return 0;
}





