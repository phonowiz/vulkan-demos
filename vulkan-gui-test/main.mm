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
#include "vulkan_wrapper/deferred_renderer.h"

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


vk::visual_mat_shared_ptr standard_mat;
vk::visual_mat_shared_ptr display_mat;
vk::visual_mat_shared_ptr mrt_mat;



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

    vk::shader_parameter::shader_params_group& vertexParams =   standard_mat->get_uniform_parameters(vk::visual_material::parameter_stage::VERTEX, 0);
    
    vertexParams["model"] = model;
    vertexParams["view"] = view;
    vertexParams["projection"] = projection;
    vertexParams["lightPosition"] = temp;


    //standard_mat->set_image_sampler(texture, "tex", vk::material::parameter_stage::FRAGMENT, 1);
    
    
    standard_mat->commit_parameters_to_gpu();
}

struct App
{
    vk::swapchain* swapchain = nullptr;
    vk::device* physical_device = nullptr;
    vk::renderer* renderer = nullptr;
    vk::deferred_renderer* deferred_renderer = nullptr;
};


App app;


void updateMVP3()
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
    
    vk::shader_parameter::shader_params_group& vertex_params =
        app.deferred_renderer->get_material()->get_uniform_parameters(vk::visual_material::parameter_stage::VERTEX, 0);
    
    vertex_params["model"] = model;
    vertex_params["view"] = view;
    vertex_params["projection"] = projection;
    vertex_params["lightPosition"] = temp;
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

void gameLoop4(vk::renderer& renderer)
{
    int i = 0;
    while(!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        updateMVP3();
        renderer.draw();
        ++i;
    }
}



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
        
        if( app.renderer )
            app.renderer->recreate_renderer() ;
        if( app.deferred_renderer)
            app.deferred_renderer->recreate_renderer() ;
        
    }
}



void updateWithOrtho()
{

    vk::shader_parameter::shader_params_group& vertexParams = display_mat->get_uniform_parameters(vk::visual_material::parameter_stage::VERTEX, 0);
    vertexParams["width"] = width;
    vertexParams["height"] = height;
    
    
    display_mat->set_image_sampler(texture, "tex", vk::visual_material::parameter_stage::FRAGMENT, 1);
    
    display_mat->commit_parameters_to_gpu();
    
}

void gameLoopOrtho(vk::renderer &renderer)
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
    
    standard_mat = material_store.GET_MAT<vk::visual_material>("standard");
    display_mat = material_store.GET_MAT<vk::visual_material>("display");
    vk::texture_2d mario(&device, "mario.png");
    
    bool deferred = true;
    if(deferred)
    {
        vk::deferred_renderer deferred_renderer(&device, window, &swapchain, material_store);
        texture = &mario;

        deferred_renderer.add_mesh(&mesh);
        deferred_renderer.init();
        
        app.physical_device = &device;
        app.swapchain = &swapchain;
        app.renderer = nullptr;
        app.deferred_renderer = &deferred_renderer;
        
        gameLoop4(deferred_renderer);
        
        deferred_renderer.destroy();
    }
    else
    {

        vk::renderer renderer(&device,window, &swapchain, standard_mat);
        //vk::renderer renderer(&device, window, &swapchain, display_mat);
        
        renderer.add_mesh(&mesh);
        //renderer.add_mesh(&plane);
        
        texture = &mario;
        app.physical_device = &device;
        app.swapchain = &swapchain;
        app.renderer = &renderer;
        app.deferred_renderer = nullptr;

        updateMVP2();
        //updateWithOrtho();
        
        renderer.init();

        gameLoop2(renderer);
        //gameLoopOrtho(renderer);
        
        renderer.destroy();
        
    }
    
    swapchain.destroy();
    material_store.destroy();
    mesh.destroy();
    plane.destroy();
    device.destroy();
    
    shutdownGlfw();
    return 0;
}





