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
#include "vulkan_wrapper/shapes/mesh.h"
#include "vulkan_wrapper/display_plane.h"
#include "vulkan_wrapper/deferred_renderer.h"
#include "vulkan_wrapper/cameras/perspective_camera.h"
#include "vulkan_wrapper/display_2d_texture_renderer.h"

///an excellent summary of vulkan can be found here:
//https://renderdoc.org/vulkan-in-30-minutes.html


GLFWwindow *window = nullptr;
VkSurfaceKHR surface;
VkSurfaceKHR surface2;

int width = 1024;
int height = 768;

void start_glfw() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_TRUE);
    constexpr int DEFAULT_VSYNC = 1;
    glfwSwapInterval(DEFAULT_VSYNC);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    
    window = glfwCreateWindow(width, height, "Vulkan Tutorial", nullptr, nullptr);
}

//render target
std::chrono::time_point game_start_time = std::chrono::high_resolution_clock::now();

void shutdown_glfw() {
    glfwDestroyWindow(window);
    glfwTerminate();
}


vk::visual_mat_shared_ptr standard_mat;
vk::visual_mat_shared_ptr mrt_mat;
vk::visual_mat_shared_ptr display_3d_tex_mat;


//note: this enum class is tied to values in the deferred renderer shader, if these change, then check that shader
//and update accordingly
enum class rendering_state
{
    DEFERRED = 0,
    THREE_D_TEXTURE
};

struct App
{
    vk::device* device = nullptr;
    vk::deferred_renderer*   deferred_renderer = nullptr;
    vk::renderer*   three_d_renderer = nullptr;
    
    vk::camera*     perspective_camera = nullptr;
    vk::camera*     three_d_texture_camera = nullptr;
    vk::camera*     ortho_camera = nullptr;
    vk::swapchain*  swapchain = nullptr;
    
    vk::deferred_renderer::rendering_state state = vk::deferred_renderer::rendering_state::FULL_RENDERING;
};


App app;

void update_3d_texture_rendering_params( vk::renderer& renderer)
{
    std::chrono::time_point frame_time = std::chrono::high_resolution_clock::now();
    float time_since_start = std::chrono::duration_cast<std::chrono::milliseconds>( frame_time - game_start_time ).count()/1000.0f;
    
    glm::mat4 model = glm::rotate(glm::mat4(1.0f), time_since_start * glm::radians(30.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    vk::shader_parameter::shader_params_group& vertex_params =   renderer.get_material()->get_uniform_parameters(vk::visual_material::parameter_stage::VERTEX, 0);
    app.three_d_texture_camera->update_view_matrix();
    

    glm::mat4 mvp = app.three_d_texture_camera->get_projection_matrix() * app.three_d_texture_camera->view_matrix * model;
    vertex_params["mvp"] = mvp;
    vertex_params["model"] = model;
    
    vk::shader_parameter::shader_params_group& fragment_params =   renderer.get_material()->get_uniform_parameters(vk::visual_material::parameter_stage::FRAGMENT, 1);
    glm::mat4 mvp_inverse = glm::inverse(app.three_d_texture_camera->get_projection_matrix() * app.three_d_texture_camera->view_matrix * model);
    
    fragment_params["mvp_inverse"] = mvp_inverse;
    fragment_params["box_eye_position"] =  glm::inverse( model) *  glm::vec4(app.three_d_texture_camera->position, 1.0f);
    fragment_params["screen_height"] = static_cast<float>(app.swapchain->_swapchain_data.swapchain_extent.width);
    fragment_params["screen_width"] = static_cast<float>(app.swapchain->_swapchain_data.swapchain_extent.height);

}


void update_renderer_parameters( vk::renderer& renderer)
{
    std::chrono::time_point frame_time = std::chrono::high_resolution_clock::now();
    float time_since_start = std::chrono::duration_cast<std::chrono::milliseconds>( frame_time - game_start_time ).count()/1000.0f;
    
    glm::mat4 model = glm::rotate(glm::mat4(1.0f), time_since_start * glm::radians(30.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    
    glm::vec4 temp =(glm::rotate(glm::mat4(1.0f), time_since_start * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)) * glm::vec4(0.0f, 3.0f, 1.0f, 0.0f));

    vk::shader_parameter::shader_params_group& vertex_params = renderer.get_material()->get_uniform_parameters(vk::visual_material::parameter_stage::VERTEX, 0);
    
    //TODO: this model matrix applies to every model being submitted for drawing.  For model flexibility, look into uniform dynamic buffers example:
    // https://github.com/SaschaWillems/Vulkan/blob/master/screenshots/dynamicuniformbuffer.jpg
    
    app.perspective_camera->update_view_matrix();
    vertex_params["model"] = model;
    vertex_params["view"] = app.perspective_camera->view_matrix;
    vertex_params["projection"] =  app.perspective_camera->get_projection_matrix();
    vertex_params["lightPosition"] = temp;
    
//    vk::shader_parameter::shader_params_group& fragment_params = renderer.get_material()->get_uniform_parameters(vk::visual_material::parameter_stage::FRAGMENT, 5);
//    fragment_params["state"] = static_cast<int>(app.state);

}

void game_loop_3d_texture(vk::renderer &renderer)
{
    int i = 0;
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        update_3d_texture_rendering_params( renderer);
        renderer.draw(*app.perspective_camera);
        ++i;
    }
}

void update_ortho_parameters(vk::renderer& renderer)
{
    vk::shader_parameter::shader_params_group& vertex_params =  renderer.get_material()->get_uniform_parameters(vk::visual_material::parameter_stage::VERTEX, 0);
    vertex_params["width"] = width;
    vertex_params["height"] = height;
}

void game_loop()
{

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
//
//        if(app.state != vk::deferred_renderer::rendering_state::THREE_D_TEXTURE )
//        {
            update_renderer_parameters( *app.deferred_renderer );
            app.deferred_renderer->draw(*app.perspective_camera);
//        }
        
//        else
//        {
//            update_3d_texture_rendering_params(*app.three_d_renderer);
//            app.three_d_renderer->draw(*app.three_d_texture_camera);
//        }
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
        
        app.deferred_renderer->recreate_renderer() ;
    }
}



void game_loop_ortho(vk::renderer &renderer)
{
    int i = 0;
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        update_ortho_parameters( renderer );
        renderer.draw(*app.perspective_camera);
        ++i;
    }
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_1 && action == GLFW_PRESS)
    {
        app.deferred_renderer->set_rendering_state(vk::deferred_renderer::rendering_state::FULL_RENDERING);
    }
    
    if (key == GLFW_KEY_2 && action == GLFW_PRESS)
    {
        app.deferred_renderer->set_rendering_state(vk::deferred_renderer::rendering_state::ALBEDO);
    }
    
    if (key == GLFW_KEY_3 && action == GLFW_PRESS)
    {
        app.deferred_renderer->set_rendering_state(vk::deferred_renderer::rendering_state::NORMALS);
    }
    
    if( key == GLFW_KEY_4 && action == GLFW_PRESS)
    {
        app.deferred_renderer->set_rendering_state(vk::deferred_renderer::rendering_state::POSITIONS);
    }
    
    if( key == GLFW_KEY_5 && action == GLFW_PRESS)
    {
        app.deferred_renderer->set_rendering_state(vk::deferred_renderer::rendering_state::DEPTH);
    }
}

int main()
{
    start_glfw();
    
    glfwSetWindowSizeCallback(window, on_window_resize);

    glfwSetKeyCallback(window, key_callback);

    vk::device device;
    
    app.device = &device;
    
    glfwCreateWindowSurface(device._instance, window, nullptr, &surface);
    device.create_logical_device(surface);

    vk::swapchain swapchain(&device, window, surface);
    
    app.swapchain = &swapchain;
    
    vk::material_store material_store;
    
    material_store.create(&device);
    
    vk::mesh mesh( "dragon.obj", &device );
    vk::mesh cube("cube.obj", &device);
    
//    vk::texture_2d mario(&device, "mario.png");
//    mario.init();

    
    standard_mat = material_store.GET_MAT<vk::visual_material>("standard");
    display_3d_tex_mat = material_store.GET_MAT<vk::visual_material>("display_3d_texture");
    
    vk::perspective_camera perspective_camera(glm::radians(60.0f),
                                              swapchain._swapchain_data.swapchain_extent.width/ swapchain._swapchain_data.swapchain_extent.height, .01f, 10.f);
    
    vk::perspective_camera three_d_texture_cam(glm::radians(60.0f),
                                              swapchain._swapchain_data.swapchain_extent.width/ swapchain._swapchain_data.swapchain_extent.height, .01f, 10.f);
    
    vk::orthographic_camera ortho_camera( 1.5f, 1.5f, 10.f);

    app.ortho_camera = &ortho_camera;
    app.ortho_camera->position = glm::vec3( 1.0f, 0.0f, -8.f);
    app.ortho_camera->forward = -app.ortho_camera->position;
    
    app.perspective_camera = &perspective_camera;
    app.perspective_camera->position = glm::vec3(1.0f, 0.0f, -5.0f);
    app.perspective_camera->forward = -perspective_camera.position;
    
    
    glm::vec3 eye(1.0f, 0.0f, -3.0f);
    app.three_d_texture_camera = &three_d_texture_cam;
    app.three_d_texture_camera->position = eye;
    app.three_d_texture_camera->forward = -eye;
    
    
    vk::deferred_renderer deferred_renderer(&device, window, &swapchain, material_store);
    vk::renderer three_d_renderer(&device, window, &swapchain, display_3d_tex_mat);
    
    app.three_d_renderer = &three_d_renderer;
    
    app.deferred_renderer = &deferred_renderer;
    
    app.deferred_renderer->add_mesh(&mesh);
    app.deferred_renderer->init();
    
    
    //vk::texture_2d* voxelizer_cam_view = deferred_renderer.get_voxelizer_cam_texture();
    //app.voxelizer_camera_renderer->show_texture(voxelizer_cam_view);
   
    vk::texture_3d* voxel_texture = deferred_renderer.get_voxel_texture();
    app.three_d_renderer->get_material()->set_image_sampler(voxel_texture, "texture_3d",
                                                            vk::visual_material::parameter_stage::FRAGMENT, 2, vk::visual_material::usage_type::COMBINED_IMAGE_SAMPLER );
    
    app.three_d_renderer->add_mesh(&cube);
    app.three_d_renderer->get_pipeline().set_depth_enable(true);
    
    app.three_d_renderer->get_pipeline().set_cullmode(vk::graphics_pipeline::cull_mode::NONE);
    app.three_d_renderer->init();
    

    game_loop();
    
    deferred_renderer.destroy();
    three_d_renderer.destroy();
    
    swapchain.destroy();
    material_store.destroy();
    mesh.destroy();
    cube.destroy();
    device.destroy();
    
    shutdown_glfw();
    return 0;
}





