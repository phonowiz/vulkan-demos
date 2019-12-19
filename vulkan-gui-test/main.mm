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


#include <MoltenVK/vk_mvk_moltenvk.h>
#include <MoltenVK/mvk_vulkan.h>

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

#include "vulkan_wrapper/core/device.h"
#include "vulkan_wrapper/core/swapchain.h"
#include "vulkan_wrapper/renderers/renderer.h"
#include "vulkan_wrapper/renderers/display_2d_texture_renderer.h"
#include "vulkan_wrapper/renderers/deferred_renderer.h"

#include "vulkan_wrapper/materials/material_store.h"
#include "vulkan_wrapper/shapes/obj_shape.h"
#include "vulkan_wrapper/shapes/cornell_box.h"

#include "vulkan_wrapper/cameras/perspective_camera.h"


#include "camera_controllers/first_person_controller.h"


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


struct App
{
    vk::device* device = nullptr;
    vk::deferred_renderer*   deferred_renderer = nullptr;
    vk::renderer*   three_d_renderer = nullptr;
    
    first_person_controller* user_controller = nullptr;
    first_person_controller* texture_3d_view_controller = nullptr;
    
    vk::camera*     perspective_camera = nullptr;
    vk::camera*     three_d_texture_camera = nullptr;
    vk::swapchain*  swapchain = nullptr;
    
    vk::deferred_renderer::rendering_state state = vk::deferred_renderer::rendering_state::FULL_RENDERING;
    
    std::vector<vk::obj_shape*> shapes;
    
    bool quit = false;
    bool render_3d_texture = false;
    
    glm::mat4 model = glm::mat4(1.0f);
    
};


App app;

void update_3d_texture_rendering_params( vk::renderer& renderer)
{
    vk::shader_parameter::shader_params_group& vertex_params =   renderer.get_material()->get_uniform_parameters(vk::visual_material::parameter_stage::VERTEX, 0);
    app.three_d_texture_camera->update_view_matrix();
    
    
    glm::mat4 mvp = app.three_d_texture_camera->get_projection_matrix() * app.three_d_texture_camera->view_matrix * glm::mat4(1.0f);
    vertex_params["mvp"] = mvp;
    vertex_params["model"] = glm::mat4(1.0f);
    
    vk::shader_parameter::shader_params_group& fragment_params =   renderer.get_material()->get_uniform_parameters(vk::visual_material::parameter_stage::FRAGMENT, 1);
    
    fragment_params["box_eye_position"] =   glm::vec4(app.three_d_texture_camera->position, 1.0f);
    fragment_params["screen_height"] = static_cast<float>(app.swapchain->_swapchain_data.swapchain_extent.width);
    fragment_params["screen_width"] = static_cast<float>(app.swapchain->_swapchain_data.swapchain_extent.height);
    
}


void update_renderer_parameters( vk::deferred_renderer& renderer)
{
    std::chrono::time_point frame_time = std::chrono::high_resolution_clock::now();
    float time_since_start = std::chrono::duration_cast<std::chrono::milliseconds>( frame_time - game_start_time ).count()/1000.0f;
    
    app.model = glm::rotate(glm::mat4(1.0f), time_since_start * glm::radians(30.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    
    //glm::vec4 temp =(glm::rotate(glm::mat4(1.0f), time_since_start * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)) * glm::vec4(0.0f, 3.0f, 1.0f, 0.0f));
    glm::vec3 temp = glm::vec3(sinf(float(time_since_start * 0.67)), sinf(float(time_since_start * 0.78)), cosf(float(time_since_start * 0.67))) * .6f;
    
    renderer._light_pos = temp;//glm::vec3(temp.x, temp.y, temp.z);
    vk::shader_parameter::shader_params_group& vertex_params = renderer.get_material()->get_uniform_parameters(vk::visual_material::parameter_stage::VERTEX, 0);
    
    app.perspective_camera->update_view_matrix();
    
    vertex_params["view"] = app.perspective_camera->view_matrix;
    vertex_params["projection"] =  app.perspective_camera->get_projection_matrix();
    vertex_params["lightPosition"] = temp;
    
    
    for( uint32_t i = 0; i < app.shapes.size(); ++i)
    {
        //app.shapes[i]->transform.mat_transform = app.model*  app.shapes[i]->transform.mat_transform;
        renderer.get_material()->get_dynamic_parameters(vk::visual_material::parameter_stage::VERTEX, 1)[i]["model"] = app.shapes[i]->transform.get_transform_matrix();
    }

    
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
    
    while (!glfwWindowShouldClose(window) && !app.quit)
    {
        glfwPollEvents();
        
        app.user_controller->update();
        app.texture_3d_view_controller->update();
        if(!app.render_3d_texture)
        {
            update_renderer_parameters( *app.deferred_renderer );
            app.deferred_renderer->draw(*app.perspective_camera);
        }
        else
        {
            app.device->wait_for_all_operations_to_finish();
            update_3d_texture_rendering_params(*app.three_d_renderer);
            app.three_d_renderer->draw(*app.three_d_texture_camera);
        }
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
        app.render_3d_texture = false;
    }
    
    if (key == GLFW_KEY_2 && action == GLFW_PRESS)
    {
        app.deferred_renderer->set_rendering_state(vk::deferred_renderer::rendering_state::ALBEDO);
        app.render_3d_texture = false;
    }
    
    if (key == GLFW_KEY_3 && action == GLFW_PRESS)
    {
        app.deferred_renderer->set_rendering_state(vk::deferred_renderer::rendering_state::NORMALS);
        app.render_3d_texture = false;
    }
    
    if( key == GLFW_KEY_4 && action == GLFW_PRESS)
    {
        app.deferred_renderer->set_rendering_state(vk::deferred_renderer::rendering_state::POSITIONS);
        app.render_3d_texture = false;
    }
    
    if( key == GLFW_KEY_5 && action == GLFW_PRESS)
    {
        app.deferred_renderer->set_rendering_state(vk::deferred_renderer::rendering_state::DEPTH);
        app.render_3d_texture = false;
    }
    if( key == GLFW_KEY_6 && action == GLFW_PRESS)
    {
        app.deferred_renderer->set_rendering_state(vk::deferred_renderer::rendering_state::AMBIENT_OCCLUSION);
        app.render_3d_texture = false;
    }
    if( key == GLFW_KEY_7 && action == GLFW_PRESS)
    {
        app.deferred_renderer->set_rendering_state(vk::deferred_renderer::rendering_state::AMBIENT_LIGHT);
        app.render_3d_texture = false;
    }
    if( key == GLFW_KEY_8 && action == GLFW_PRESS)
    {
        app.render_3d_texture = true;
    }
    
    if( key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        app.quit = true;
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
    
    glfwSetCursorPos(window, width * .5f, height * .5f);
    vk::swapchain swapchain(&device, window, surface);
    
    app.swapchain = &swapchain;
    
    vk::material_store material_store;
    
    material_store.create(&device);
    
    vk::texture_2d mario(&device, "mario_small.png");
    mario.set_enable_mipmapping(true);
    mario.init();
    
    vk::obj_shape buddha(&device, "dragon.obj");
    vk::obj_shape cube(&device, "cube.obj");
    vk::cornell_box cornell_box(&device);
    
    buddha.set_diffuse(glm::vec3(.50f, 0.50f, .500f));
    buddha.create();
    cube.create();
    
    buddha.transform.position = glm::vec3(0.25f, -.5f, .00f);
    buddha.transform.scale = glm::vec3(1.5f, 1.5f, 1.5f);
    buddha.transform.update_transform_matrix();
    
    cornell_box.transform.position = glm::vec3(0.0f, 0.00f, 0.0f);
    cornell_box.transform.rotation.y = 3.14159f;
    cornell_box.transform.update_transform_matrix();
    cornell_box.create();
    
    standard_mat = material_store.GET_MAT<vk::visual_material>("standard");
    display_3d_tex_mat = material_store.GET_MAT<vk::visual_material>("display_3d_texture");
    
    vk::perspective_camera perspective_camera(glm::radians(45.0f),
                                              swapchain._swapchain_data.swapchain_extent.width/ swapchain._swapchain_data.swapchain_extent.height, .01f, 100.0f);
    
    vk::perspective_camera three_d_texture_cam(glm::radians(45.0f),
                                               swapchain._swapchain_data.swapchain_extent.width/ swapchain._swapchain_data.swapchain_extent.height, .01f, 100.0f);
    
    app.perspective_camera = &perspective_camera;
    app.perspective_camera->position = glm::vec3(0.0f, .2f, -5.0f);
    app.perspective_camera->forward = -perspective_camera.position;
    
    
    glm::vec3 eye(0.0f, 0.0f, -3.0f);
    app.three_d_texture_camera = &three_d_texture_cam;
    app.three_d_texture_camera->position = eye;
    app.three_d_texture_camera->forward = -eye;
    
    
    vk::deferred_renderer deferred_renderer(&device, window, &swapchain, material_store);
    vk::renderer three_d_renderer(&device, window, &swapchain, display_3d_tex_mat);
    vk::display_2d_texture_renderer display_renderer(&device, window, &swapchain, material_store);
    
    display_renderer.get_pipeline().set_depth_enable(false);
    display_renderer.show_texture(&mario);
    display_renderer.init();
    
    
    app.three_d_renderer = &three_d_renderer;
    
    app.deferred_renderer = &deferred_renderer;
    
    app.deferred_renderer->add_shape(&cornell_box);
    app.deferred_renderer->add_shape(&buddha);

    app.shapes.push_back(&cornell_box);
    app.shapes.push_back(&buddha);

    
    app.deferred_renderer->init();
    
    vk::texture_3d* voxel_texture = deferred_renderer.get_voxel_texture(1);
    app.three_d_renderer->get_material()->set_image_sampler(voxel_texture, "texture_3d",
                                                            vk::visual_material::parameter_stage::FRAGMENT, 2, vk::visual_material::usage_type::COMBINED_IMAGE_SAMPLER );
    
    app.three_d_renderer->add_shape(&cube);
    app.three_d_renderer->get_pipeline().set_depth_enable(true);
    
    app.three_d_renderer->get_pipeline().set_cullmode(vk::graphics_pipeline::cull_mode::NONE);
    app.three_d_renderer->init();
    
    
    first_person_controller user_controler( app.perspective_camera, window);
    first_person_controller  texture_3d_view_controller(app.three_d_texture_camera, window);
    
    app.user_controller = &user_controler;
    app.texture_3d_view_controller = &texture_3d_view_controller;
    
    game_loop();
    //game_loop_ortho(display_renderer);
    
    mario.destroy();
    deferred_renderer.destroy();
    three_d_renderer.destroy();
    display_renderer.destroy();
    
    swapchain.destroy();
    material_store.destroy();
    buddha.destroy();
    cube.destroy();
    cornell_box.destroy();
    device.destroy();
    
    shutdown_glfw();
    return 0;
}





