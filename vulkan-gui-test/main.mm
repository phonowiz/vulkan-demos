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
#include <glm/mat4x4.hpp>
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

void start_glfw() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_TRUE);
    
    window = glfwCreateWindow(width, height, "Vulkan Tutorial", nullptr, nullptr);
}

//render target
std::chrono::time_point game_start_time = std::chrono::high_resolution_clock::now();



void shutdown_glfw() {
    glfwDestroyWindow(window);
    glfwTerminate();
}


vk::visual_mat_shared_ptr standard_mat;
vk::visual_mat_shared_ptr display_mat;
vk::visual_mat_shared_ptr mrt_mat;
vk::visual_mat_shared_ptr display_3d_tex_mat;

enum class rendering_state
{
    DEFERRED,
    STANDARD,
    TWO_D_TEXTURE,
    THREE_D_TEXTURE
};

struct App
{
    vk::device* device = nullptr;
    vk::renderer* renderer = nullptr;
    rendering_state state = rendering_state::STANDARD;
};


App app;

vk::texture_2d* texture = nullptr;


void update_3d_texture_rendering_params( vk::renderer& renderer)
{
    std::chrono::time_point frame_time = std::chrono::high_resolution_clock::now();
    float time_since_start = std::chrono::duration_cast<std::chrono::milliseconds>( frame_time - game_start_time ).count()/1000.0f;
    
    glm::mat4 scale;
    glm::scale(scale, glm::vec3(1.0f, 1.0f, 1.0f));
    
    glm::vec3 eye(1.0f, 0.0f, -3.0f);
    glm::vec3 look_at_point(0.0f, 0.0f, 0.0f);
    glm::vec3 up(0.0f, 1.0f, 0.0f);
    
    glm::mat4 model = scale * glm::rotate(glm::mat4(1.0f), time_since_start * glm::radians(30.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 view = glm::lookAt(eye, look_at_point, up);
    glm::mat4 projection = glm::perspective(glm::radians(60.0f), width/(float)height, 0.01f, 10.0f);
    
    /*
     GLM was originally designed for OpenGL, where the Y coordinate of the clip coordinates is inverted.
     The easiest way to compensate for that is to flip the sign on the scaling factor of the Y axis in the projection matrix.
     If you don't do this, then the image will be rendered upside down.
     */
    projection[1][1] *= -1.0f;
    
    //glm::vec4 temp =(glm::rotate(glm::mat4(1.0f), time_since_start * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)) * glm::vec4(0.0f, 3.0f, 1.0f, 0.0f));
    
    vk::shader_parameter::shader_params_group& vertex_params =   renderer.get_material()->get_uniform_parameters(vk::visual_material::parameter_stage::VERTEX, 0);
    
    vertex_params["mvp"] = projection * view * model;
    vertex_params["model"] = model;
    
    vk::shader_parameter::shader_params_group& fragment_params =   renderer.get_material()->get_uniform_parameters(vk::visual_material::parameter_stage::FRAGMENT, 1);
    fragment_params["eye_world_position"] = eye;
}


void update_renderer_parameters( vk::renderer& renderer)
{
    std::chrono::time_point frame_time = std::chrono::high_resolution_clock::now();
    float time_since_start = std::chrono::duration_cast<std::chrono::milliseconds>( frame_time - game_start_time ).count()/1000.0f;
    
    glm::mat4 scale;
    glm::scale(scale, glm::vec3(1.0f, 1.0f, 1.0f));
    
    glm::vec3 eye(1.0f, 0.0f, -1.0f);
    glm::vec3 look_at_point(0.0f, 0.0f, 0.0f);
    glm::vec3 up(0.0f, 1.0f, 0.0f);
    
    glm::mat4 model = scale * glm::rotate(glm::mat4(1.0f), time_since_start * glm::radians(30.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 view = glm::lookAt(eye, look_at_point, up);
    glm::mat4 projection = glm::perspective(glm::radians(60.0f), width/(float)height, 0.01f, 10.0f);
    
    /*
     GLM was originally designed for OpenGL, where the Y coordinate of the clip coordinates is inverted.
     The easiest way to compensate for that is to flip the sign on the scaling factor of the Y axis in the projection matrix.
     If you don't do this, then the image will be rendered upside down.
     */
    projection[1][1] *= -1.0f;
    
    glm::vec4 temp =(glm::rotate(glm::mat4(1.0f), time_since_start * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)) * glm::vec4(0.0f, 3.0f, 1.0f, 0.0f));

    vk::shader_parameter::shader_params_group& vertex_params =   renderer.get_material()->get_uniform_parameters(vk::visual_material::parameter_stage::VERTEX, 0);
    
    vertex_params["model"] = model;
    vertex_params["view"] = view;
    vertex_params["projection"] = projection;
    vertex_params["lightPosition"] = temp;

}

void game_loop_3d_texture(vk::renderer &renderer)
{
    int i = 0;
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        update_3d_texture_rendering_params( renderer);
        renderer.draw();
        ++i;
    }
}

void game_loop(vk::renderer &renderer)
{
    int i = 0;
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        update_renderer_parameters( renderer);
        renderer.draw();
        ++i;
    }
}

void on_window_resize(GLFWwindow * window, int w, int h)
{
    if( w != 0 && h != 0)
    {
        VkSurfaceCapabilitiesKHR surface_capabilities {};
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(app.device->_physical_device, surface, &surface_capabilities);
        
        w = std::min(w, static_cast<int>(surface_capabilities.maxImageExtent.width));
        h = std::min(h, static_cast<int>(surface_capabilities.maxImageExtent.height));
        
        w = std::max(w, static_cast<int>(surface_capabilities.minImageExtent.width));
        h = std::max(h, static_cast<int>(surface_capabilities.minImageExtent.height));
        
        width = w;
        height = h;
        
        app.renderer->recreate_renderer() ;
    }
}

void update_ortho_parameters(vk::renderer& renderer)
{

    vk::shader_parameter::shader_params_group& vertex_params =  renderer.get_material()->get_uniform_parameters(vk::visual_material::parameter_stage::VERTEX, 0);
    vertex_params["width"] = width;
    vertex_params["height"] = height;

    renderer.get_material()->set_image_sampler(texture, "tex", vk::visual_material::parameter_stage::FRAGMENT, 1, vk::resource::usage_type::COMBINED_IMAGE_SAMPLER);
    
}

void game_loop_ortho(vk::renderer &renderer)
{
    int i = 0;
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        update_ortho_parameters( renderer );
        renderer.draw();
        ++i;
    }
}

int main()
{
    start_glfw();
    
    glfwSetWindowSizeCallback(window, on_window_resize);
    const int DEFAULT_VSYNC = 1;
    glfwSwapInterval(DEFAULT_VSYNC);
    
    vk::device device;
    
    app.device = &device;
    
    VkResult res = glfwCreateWindowSurface(device._instance, window, nullptr, &surface);
    assert(res == VK_SUCCESS);
    
    device.create_logical_device(surface);
    
    vk::swapchain swapchain(&device, window, surface);
    vk::material_store material_store;
    
    material_store.create(&device);
    
    vk::mesh mesh( "dragon.obj", &device );
    vk::mesh cube("cube.obj", &device);
    vk::display_plane plane(&device);
    
    standard_mat = material_store.GET_MAT<vk::visual_material>("standard");
    display_mat = material_store.GET_MAT<vk::visual_material>("display");
    display_3d_tex_mat = material_store.GET_MAT<vk::visual_material>("display_3d_texture");

    
    app.state = rendering_state::THREE_D_TEXTURE;
    switch( app.state )
    {
        case rendering_state::TWO_D_TEXTURE:
        {

            vk::texture_2d mario(&device, "mario.png");
            texture = &mario;
            vk::renderer renderer(&device, window, &swapchain, display_mat);
            app.renderer = &renderer;

            renderer.add_mesh(&plane);
            renderer.init();
            
            game_loop_ortho(renderer);
            renderer.destroy();
            break;
        }
        case rendering_state::DEFERRED:
        {
            vk::deferred_renderer deferred_renderer(&device, window, &swapchain, material_store);
            
            app.renderer = &deferred_renderer;
            
            deferred_renderer.add_mesh(&mesh);
            deferred_renderer.init();
            
            game_loop(deferred_renderer);
            deferred_renderer.destroy();
            break;
        }
        case rendering_state::THREE_D_TEXTURE:
        {

            vk::renderer renderer(&device, window, &swapchain, display_3d_tex_mat);
            app.renderer = &renderer;
            //vk::texture_3d tex_3d(&device, 256u, 256u, 256u);
            //renderer.get_material()->set_image_sampler(&tex_3d, "tex_3d", vk::visual_material::parameter_stage::FRAGMENT, 1, vk::resource::usage_type::COMBINED_IMAGE_SAMPLER);
            

            renderer.add_mesh(&cube);
            renderer.init();
            
            game_loop_3d_texture(renderer);
            renderer.destroy();
        }
        default:
        {
            vk::renderer renderer(&device,window, &swapchain, standard_mat);
            
            app.renderer = &renderer;
            
            renderer.add_mesh(&mesh);
            
            renderer.init();
            
            game_loop(renderer);
            renderer.destroy();
            break;
        }
    }
    
    swapchain.destroy();
    material_store.destroy();
    mesh.destroy();
    plane.destroy();
    cube.destroy();
    device.destroy();
    
    shutdown_glfw();
    return 0;
}





