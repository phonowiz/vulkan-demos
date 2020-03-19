// VulkanTutorial.cpp : Defines the entry point for the console application.
//

//#include "stdafx.h"
#define VK_USE_PLATFORM_MACOS_MVK
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
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
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <chrono>

#include "vulkan_wrapper/core/device.h"
#include "vulkan_wrapper/core/glfw_swapchain.h"
#include "vulkan_wrapper/renderers/renderer.h"
#include "vulkan_wrapper/renderers/display_2d_texture_renderer.h"
#include "vulkan_wrapper/renderers/deferred_renderer.h"

#include "vulkan_wrapper/materials/material_store.h"
#include "vulkan_wrapper/shapes/obj_shape.h"
#include "vulkan_wrapper/shapes/cornell_box.h"

#include "vulkan_wrapper/cameras/perspective_camera.h"
#include "camera_controllers/first_person_controller.h"

#include <filesystem>

namespace fs = std::filesystem;



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
    
    window = glfwCreateWindow(width, height, "Rafael's Demo", nullptr, nullptr);
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
    vk::renderer<vk::glfw_present_texture, 1>*   three_d_renderer = nullptr;
    vk::display_2d_texture_renderer * display_renderer = nullptr;
    
    first_person_controller* user_controller = nullptr;
    first_person_controller* texture_3d_view_controller = nullptr;
    
    vk::camera*     perspective_camera = nullptr;
    vk::camera*     three_d_texture_camera = nullptr;
    vk::glfw_swapchain*  swapchain = nullptr;
    
    vk::deferred_renderer::rendering_mode state = vk::deferred_renderer::rendering_mode::FULL_RENDERING;
    
    std::vector<vk::obj_shape*> shapes;
     
    enum class render_mode
    {
        RENDER_3D_TEXTURE,
        RENDER_VOXEL_CAM_TEXTURE,
        RENDER_DEFFERED
    };
    
    bool quit = false;
    render_mode mode = render_mode::RENDER_DEFFERED;
    glm::mat4 model = glm::mat4(1.0f);
    
};


App app;

void update_3d_texture_rendering_params( vk::renderer<vk::glfw_present_texture, 1>& three_d_renderer, int next_swap)
{

    vk::shader_parameter::shader_params_group& vertex_params =
        three_d_renderer.get_render_pass().get_subpass(0).get_pipeline(next_swap).get_uniform_parameters(vk::visual_material::parameter_stage::VERTEX, 0);
    app.three_d_texture_camera->update_view_matrix();


    glm::mat4 mvp = app.three_d_texture_camera->get_projection_matrix() * app.three_d_texture_camera->view_matrix * glm::mat4(1.0f);
    
    vertex_params["mvp"] = mvp;
    vertex_params["model"] = glm::mat4(1.0f);

    vk::shader_parameter::shader_params_group& fragment_params = three_d_renderer.get_render_pass().get_subpass(0).
    get_pipeline(next_swap).get_uniform_parameters(vk::visual_material::parameter_stage::FRAGMENT, 1);
    
    fragment_params["box_eye_position"] =   glm::vec4(app.three_d_texture_camera->position, 1.0f);
    fragment_params["screen_height"] = static_cast<float>(app.swapchain->get_vk_swap_extent().width);
    fragment_params["screen_width"] = static_cast<float>(app.swapchain->get_vk_swap_extent().height);
    
    three_d_renderer.set_next_swapchain_id(next_swap);
}


void update_renderer_parameters( vk::deferred_renderer& renderer)
{
    std::chrono::time_point frame_time = std::chrono::high_resolution_clock::now();
    float time_since_start = std::chrono::duration_cast<std::chrono::milliseconds>( frame_time - game_start_time ).count()/1000.0f;
    
    app.model = glm::rotate(glm::mat4(1.0f), time_since_start * glm::radians(30.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    
    glm::vec3 temp = glm::vec3(sinf(float(time_since_start * 0.67)), sinf(float(time_since_start * 0.78)), cosf(float(time_since_start * 0.67))) * .6f;
    
    renderer._light_pos = temp;
    static int32_t next_frame = -1;
    next_frame = (next_frame + 1) % vk::glfw_swapchain::NUM_SWAPCHAIN_IMAGES;
    vk::shader_parameter::shader_params_group& vertex_params = renderer.get_mrt_uniform_params(vk::visual_material::parameter_stage::VERTEX,0, 0, next_frame);

    
    app.perspective_camera->update_view_matrix();
    
    vertex_params["view"] = app.perspective_camera->view_matrix;
    vertex_params["projection"] =  app.perspective_camera->get_projection_matrix();
    for( uint32_t i = 0; i < app.shapes.size(); ++i)
    {
        renderer.get_mrt_dynamic_params(vk::visual_material::parameter_stage::VERTEX, 0,1, next_frame)[i]["model"] = app.shapes[i]->transform.get_transform_matrix();
    }
}

void update_ortho_parameters(vk::renderer<vk::glfw_present_texture, 1>& renderer, int32_t next_frame)
{
    if(next_frame != -1)
        renderer.set_next_swapchain_id(next_frame);
}

void game_loop()
{
    int next_swap = 0;
    while (!glfwWindowShouldClose(window) && !app.quit)
    {
        glfwPollEvents();
        app.user_controller->update();
        app.texture_3d_view_controller->update();
        if(app.mode == App::render_mode::RENDER_DEFFERED)
        {
            update_renderer_parameters( *app.deferred_renderer );
            app.deferred_renderer->draw(*app.perspective_camera);
            next_swap = app.deferred_renderer->get_current_swapchain_image();
        }
        else if (app.mode == App::render_mode::RENDER_3D_TEXTURE)
        {
            app.device->wait_for_all_operations_to_finish();
            next_swap = ++next_swap % 3;

            update_3d_texture_rendering_params(*app.three_d_renderer, next_swap );
            app.three_d_renderer->draw(*app.three_d_texture_camera);
        }
        else
        {
            next_swap = ++next_swap % 3;
            app.device->wait_for_all_operations_to_finish();
            update_ortho_parameters(*app.display_renderer, next_swap);
            app.display_renderer->draw(*app.perspective_camera);
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



void game_loop_ortho(vk::renderer<vk::glfw_present_texture, 1> &renderer)
{
    int i = 0;

    while (!glfwWindowShouldClose(window)) {
        static int32_t current_index = -1;
        current_index = (current_index + 1) % vk::glfw_swapchain::NUM_SWAPCHAIN_IMAGES;
        glfwPollEvents();
        update_ortho_parameters( renderer , current_index);
        renderer.draw(*app.perspective_camera);
        ++i;
    }
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    
    if (key == GLFW_KEY_1 && action == GLFW_PRESS)
    {
        app.deferred_renderer->set_rendering_state(vk::deferred_renderer::rendering_mode::FULL_RENDERING);
        app.mode = App::render_mode::RENDER_DEFFERED;
    }
    
    if (key == GLFW_KEY_2 && action == GLFW_PRESS)
    {
        app.deferred_renderer->set_rendering_state(vk::deferred_renderer::rendering_mode::ALBEDO);
        app.mode = App::render_mode::RENDER_DEFFERED;
    }
    
    if (key == GLFW_KEY_3 && action == GLFW_PRESS)
    {
        app.deferred_renderer->set_rendering_state(vk::deferred_renderer::rendering_mode::NORMALS);
        app.mode = App::render_mode::RENDER_DEFFERED;
    }
    
    if( key == GLFW_KEY_4 && action == GLFW_PRESS)
    {
        app.deferred_renderer->set_rendering_state(vk::deferred_renderer::rendering_mode::POSITIONS);
        app.mode = App::render_mode::RENDER_DEFFERED;
    }
    
    if( key == GLFW_KEY_5 && action == GLFW_PRESS)
    {
        app.deferred_renderer->set_rendering_state(vk::deferred_renderer::rendering_mode::DEPTH);
        app.mode = App::render_mode::RENDER_DEFFERED;
    }
    if( key == GLFW_KEY_6 && action == GLFW_PRESS)
    {
        app.deferred_renderer->set_rendering_state(vk::deferred_renderer::rendering_mode::AMBIENT_OCCLUSION);
        app.mode = App::render_mode::RENDER_DEFFERED;;
    }
    if( key == GLFW_KEY_7 && action == GLFW_PRESS)
    {
        app.deferred_renderer->set_rendering_state(vk::deferred_renderer::rendering_mode::AMBIENT_LIGHT);
        app.mode = App::render_mode::RENDER_DEFFERED;;
    }
    if( key == GLFW_KEY_8 && action == GLFW_PRESS)
    {
        app.mode = App::render_mode::RENDER_3D_TEXTURE;
    }
    if( key == GLFW_KEY_9 && action == GLFW_PRESS)
    {
        app.deferred_renderer->set_rendering_state(vk::deferred_renderer::rendering_mode::DIRECT_LIGHT);
        app.mode = App::render_mode::RENDER_DEFFERED;
    }
    if( key == GLFW_KEY_O && action == GLFW_PRESS)
    {
        vk::shader_parameter::shader_params_group& vertex_params =  app.display_renderer->get_uniform_params(0,vk::visual_material::parameter_stage::VERTEX, 0);
        vertex_params["width"] = vk::deferred_renderer::VOXEL_CUBE_WIDTH ;
        vertex_params["height"] = vk::deferred_renderer::VOXEL_CUBE_HEIGHT;
        app.mode = App::render_mode::RENDER_VOXEL_CAM_TEXTURE;
    }
    
    if( key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        app.quit = true;
    }
}

int main()
{
    std::cout << std::endl;
    std::cout << "working directory " << fs::current_path() << std::endl;
    
    start_glfw();
    
    glfwSetWindowSizeCallback(window, on_window_resize);
    glfwSetKeyCallback(window, key_callback);
    
    vk::device device;
    
    app.device = &device;
    
    glfwCreateWindowSurface(device._instance, window, nullptr, &surface);
    device.create_logical_device(surface);
    
    //glfwSetCursorPos(window, width * .5f, height * .5f);
    vk::glfw_swapchain swapchain(&device, window, surface);
    
    app.swapchain = &swapchain;
    
    vk::material_store material_store;
    
    material_store.create(&device);
    
    vk::texture_2d mario(&device, "mario_small.png");
    mario.set_enable_mipmapping(true);
    mario.init();
    
    vk::obj_shape model(&device, "dragon.obj");
    vk::obj_shape cube(&device, "cube.obj");
    vk::cornell_box cornell_box(&device);
    
    cube.create();
    model.set_diffuse(glm::vec3(.00f, 0.00f, .80f));
    model.create();

    
    model.transform.position = glm::vec3(0.5f, -.50f, .0f);
    model.transform.scale = glm::vec3(1.2f, 1.2f, 1.2f);
    model.transform.update_transform_matrix();
    
    cornell_box.transform.position = glm::vec3(0.0f, 0.0f, 0.0f);
    cornell_box.transform.rotation.y = 3.14159f;
    cornell_box.transform.update_transform_matrix();
    cornell_box.create();
    
    float aspect = static_cast<float>(swapchain.get_vk_swap_extent().width)/ static_cast<float>(swapchain.get_vk_swap_extent().height);
    vk::perspective_camera perspective_camera(glm::radians(45.0f),
                                              aspect, .01f, 100.0f);
    
    vk::perspective_camera three_d_texture_cam(glm::radians(45.0f),
                                               aspect, .01f, 50.0f);
    
    app.perspective_camera = &perspective_camera;
    app.perspective_camera->position = glm::vec3(0.0f, .2f, -5.0f);
    app.perspective_camera->forward = -perspective_camera.position;
    
    
    glm::vec3 eye(0.0f, 0.0f, -3.0f);
    app.three_d_texture_camera = &three_d_texture_cam;
    app.three_d_texture_camera->position = eye;
    app.three_d_texture_camera->forward = -eye;
    
    
    app.shapes.push_back(&cornell_box);
    app.shapes.push_back(&model);
    
    vk::deferred_renderer deferred_renderer(&device, window, &swapchain, material_store, app.shapes);
    vk::renderer<vk::glfw_present_texture,1> three_d_renderer(&device, window, &swapchain, material_store, "display_3d_texture");
    vk::display_2d_texture_renderer display_renderer(&device, window, &swapchain, material_store);
    
    //display_renderer.get_render_pass().get_subpass(0).set_depth_enable(false);
    //display_renderer.show_texture(deferred_renderer.get_voxelizer_cam_texture());
    for( int i = 0; i < vk::glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++i)
    {
        vk::shader_parameter::shader_params_group& vertex_params = display_renderer.get_pipeline(i, 0).
                                get_uniform_parameters(vk::visual_material::parameter_stage::VERTEX, 0);
        vertex_params["width"] = vk::deferred_renderer::VOXEL_CUBE_WIDTH ;
        vertex_params["height"] = vk::deferred_renderer::VOXEL_CUBE_HEIGHT;
    }

    display_renderer.show_texture(&mario);
    display_renderer.init();
    
    for( uint32_t i = 0; i < vk::glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++i)
    {
        display_renderer.get_render_pass().create(i);
    }
    app.display_renderer = &display_renderer;
    app.three_d_renderer = &three_d_renderer;
    app.deferred_renderer = &deferred_renderer;

    app.deferred_renderer->init();

    std::array<vk::texture_3d, vk::glfw_swapchain::NUM_SWAPCHAIN_IMAGES>& voxel_texture = deferred_renderer.get_voxel_texture();
    
    app.three_d_renderer->get_render_pass().get_subpass(0).set_image_sampler(voxel_texture, "texture_3d",
                                                                             vk::visual_material::parameter_stage::FRAGMENT, 2, vk::visual_material::usage_type::COMBINED_IMAGE_SAMPLER);
    
    app.three_d_renderer->get_render_pass().add_object(&cube);
    //app.three_d_renderer->add_shape(&cube);
    //app.three_d_renderer->get_render_pass().get_subpass(0).set_depth_enable(true);
    static const uint32_t DEPTH = 1;
    app.three_d_renderer->get_render_pass().get_subpass(0).add_output_attachment(DEPTH);
    
    app.three_d_renderer->get_render_pass().get_subpass(0).set_cull_mode(vk::standard_pipeline::cull_mode::NONE);
    
    app.three_d_renderer->init();
    for( int i = 0; i < vk::glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++i)
    {
        vk::shader_parameter::shader_params_group& vertex_params =
            three_d_renderer.get_render_pass().get_subpass(0).get_pipeline(i).get_uniform_parameters(vk::visual_material::parameter_stage::VERTEX, 0);

        //glm::mat4 mvp = app.three_d_texture_camera->get_projection_matrix() * app.three_d_texture_camera->view_matrix * glm::mat4(1.0f);
        vertex_params["mvp"] = glm::mat4(1.0f);
        vertex_params["model"] = glm::mat4(1.0f);

        vk::shader_parameter::shader_params_group& fragment_params = three_d_renderer.get_render_pass().get_subpass(0).
        get_pipeline(i).get_uniform_parameters(vk::visual_material::parameter_stage::FRAGMENT, 1);
        
        fragment_params["box_eye_position"] =   glm::vec4(1.0f);
        fragment_params["screen_height"] = (0.0f);
        fragment_params["screen_width"] = (.0f);
        
        app.three_d_renderer->get_render_pass().create(i);
    }
    
    first_person_controller user_controler( app.perspective_camera, window);
    first_person_controller  texture_3d_view_controller(app.three_d_texture_camera, window);
    
    app.user_controller = &user_controler;
    app.texture_3d_view_controller = &texture_3d_view_controller;
    
    app.user_controller->update();
    app.texture_3d_view_controller->update();
    
    game_loop();
    //game_loop_ortho(display_renderer);
    
    
    device.wait_for_all_operations_to_finish();
    mario.destroy();
    deferred_renderer.destroy();
    three_d_renderer.destroy();
    display_renderer.destroy();
    
    material_store.destroy();
    model.destroy();
    cube.destroy();
    cornell_box.destroy();
    swapchain.destroy();
    
    vkDestroySurfaceKHR(device._instance, surface, nullptr);
    device.destroy();
    

    shutdown_glfw();
    return 0;
}
