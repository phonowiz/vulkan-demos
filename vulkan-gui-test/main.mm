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


#include <assert.h>
#include <vector>
#include <array>
#include <algorithm>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>

#include "vulkan_wrapper/device.h"
#include "vulkan_wrapper/renderer.h"
#include "vulkan_wrapper/swapchain.h"
#include "vulkan_wrapper/material_store.h"
#include "vulkan_wrapper/mesh.h"
#include "vulkan_wrapper/display_plane.h"

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
}

//render target
std::chrono::time_point gameStartTime = std::chrono::high_resolution_clock::now();



void shutdownGlfw() {
    glfwDestroyWindow(window);
    glfwTerminate();
}


vk::material_shared_ptr standard_mat;
vk::material_shared_ptr display_mat;



vk::texture_2d* texture = nullptr;

void updateMVP2()
{
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

    vk::shader_parameter::shader_params_group& vertexParams =   standard_mat->get_uniform_parameters(vk::material::parameter_stage::VERTEX, 0);
    
    vertexParams["model"] = model;
    vertexParams["view"] = view;
    vertexParams["projection"] = projection;
    vertexParams["lightPosition"] = temp;


    standard_mat->set_image_sampler(texture, "tex", vk::material::parameter_stage::FRAGMENT, 1);
    
    
    standard_mat->commit_parameters_to_gpu();
}

void gameLoop2(vk::renderer &renderer)
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
    vk::swapchain* swapchain = nullptr;
    vk::device* physical_device = nullptr;
    vk::renderer* renderer = nullptr;
};


App app;

void onWindowResized2(GLFWwindow * window, int w, int h)
{
    if( w != 0 && h != 0)
    {
        
        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(app.physical_device->_physical_device, surface, &surfaceCapabilities);
        
        w = std::min(w, static_cast<int>(surfaceCapabilities.maxImageExtent.width));
        h = std::min(h, static_cast<int>(surfaceCapabilities.maxImageExtent.height));
        
        w = std::max(w, static_cast<int>(surfaceCapabilities.minImageExtent.width));
        h = std::max(h, static_cast<int>(surfaceCapabilities.minImageExtent.height));
        
        width = w;
        height = h;
        app.renderer->recreate_renderer();
        
    }
}



void updateWithOrtho()
{

    vk::shader_parameter::shader_params_group& vertexParams = display_mat->get_uniform_parameters(vk::material::parameter_stage::VERTEX, 0);
    vertexParams["width"] = width;
    vertexParams["height"] = height;
    
    
    display_mat->set_image_sampler(texture, "tex", vk::material::parameter_stage::FRAGMENT, 1);
    
    display_mat->commit_parameters_to_gpu();
    
}

void gameLoop3(vk::renderer &renderer)
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
    
    vk::device device;
    
    VkResult res = glfwCreateWindowSurface(device._instance, window, nullptr, &surface);
    assert(res == VK_SUCCESS);
    
    device.create_logical_device(surface);
    
    vk::swapchain swapchain(&device, window, surface);
    vk::material_store material_store;
    
    material_store.create(&device);
    
    vk::mesh mesh( "dragon.obj", &device );
    
    vk::display_plane plane(&device);
    bool deferred = false;
    
    standard_mat = material_store.GET_MAT<vk::material>("standard");
    display_mat = material_store.GET_MAT<vk::material>("display");
    
    vk::renderer renderer(&device,window, &swapchain, standard_mat);
    
    if(!deferred)
    {

        //vk::Texture texture(&device, "mario.png");
        

        plane.create();
        
        vk::texture_2d mario(&device, "mario.png");
        texture = &mario;
        updateMVP2();
        
        //updateWithOrtho();

        
        //renderer.addMesh(&plane);
        renderer.add_mesh(&mesh);
        renderer.init();
        
        app.physical_device = &device;
        app.swapchain = &swapchain;
        app.renderer = &renderer;
        
        gameLoop2(renderer);
        //gameLoop3(renderer);
    }
    else
    {
        vk::texture_2d position(&device, swapchain._swapchain_data.swapchain_extent.width, swapchain._swapchain_data.swapchain_extent.height);
        vk::texture_2d albedo(&device, swapchain._swapchain_data.swapchain_extent.width, swapchain._swapchain_data.swapchain_extent.height);
        vk::texture_2d depth(&device, swapchain._swapchain_data.swapchain_extent.width, swapchain._swapchain_data.swapchain_extent.height);
        
        
    }

    
    device.wait_for_all_operations_to_finish();
    swapchain.destroy();
    material_store.destroy();
    mesh.destroy();
    plane.destroy();
    renderer.destroy();
    device.destroy();
    
    shutdownGlfw();
    return 0;
}





