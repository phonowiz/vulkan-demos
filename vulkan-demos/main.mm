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

#include "vulkan_wrapper/materials/material_store.h"
#include "vulkan_wrapper/shapes/obj_shape.h"
#include "vulkan_wrapper/shapes/cornell_box.h"

#include "vulkan_wrapper/cameras/perspective_camera.h"
#include "camera_controllers/first_person_controller.h"

#include "graph_nodes/graphics_nodes/display_texture_2d.h"
#include "graph_nodes/graphics_nodes/display_texture_3d.h"
#include "graph_nodes/graphics_nodes/vsm.cpp"
#include "graph_nodes/graphics_nodes/gaussian_blur.cpp"

#include "graph_nodes/compute_nodes/mip_map_3d_texture.hpp"
#include "graph_nodes/graphics_nodes/voxelize.h"
#include "graph_nodes/compute_nodes/clear_3d_texture.hpp"
#include "graph_nodes/graphics_nodes/mrt.h"

#include "new_operators.h"
#include "graph.h"

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

struct App
{
    vk::device* device = nullptr;

    vk::graph<1> * graph = nullptr;
    
    vk::graph<4> * voxel_graph = nullptr;
    mrt<4> * mrt_node = nullptr;
    display_texture_3d<4> * debug_node_3d = nullptr;
    
    first_person_controller* user_controller = nullptr;
    first_person_controller* texture_3d_view_controller = nullptr;
    
    vk::camera*     perspective_camera = nullptr;
    vk::camera*     three_d_texture_camera = nullptr;
    vk::glfw_swapchain*  swapchain = nullptr;
    
    std::vector<vk::obj_shape*> shapes;
     
    
    bool quit = false;
    glm::mat4 model = glm::mat4(1.0f);
    
};


App app;


void game_loop()
{
    int next_swap = 0;
    while (!glfwWindowShouldClose(window) && !app.quit)
    {
        glfwPollEvents();
        app.user_controller->update();
        app.texture_3d_view_controller->update();

        app.voxel_graph->update(*app.perspective_camera, next_swap);
        app.voxel_graph->record(next_swap);
        app.voxel_graph->execute(next_swap);
        next_swap = ++next_swap % vk::NUM_SWAPCHAIN_IMAGES;
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
    }
}



void game_loop_ortho()
{
    int i = 0;

    while (!glfwWindowShouldClose(window)) {
        static int32_t current_index = -1;
        current_index = (current_index + 1) % vk::glfw_swapchain::NUM_SWAPCHAIN_IMAGES;
        glfwPollEvents();

        app.graph->update(*app.perspective_camera, current_index);
        app.graph->record(current_index);
        app.graph->execute(current_index);
        ++i;
    }
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_1 && action == GLFW_PRESS)
    {
        app.debug_node_3d->set_active(false);
        app.mrt_node->set_rendering_state(mrt<4>::rendering_mode::FULL_RENDERING);
    }
    
    if (key == GLFW_KEY_2 && action == GLFW_PRESS)
    {
        app.debug_node_3d->set_active(false);
        app.mrt_node->set_rendering_state(mrt<4>::rendering_mode::ALBEDO);
    }
    
    if (key == GLFW_KEY_3 && action == GLFW_PRESS)
    {
        app.debug_node_3d->set_active(false);
        app.mrt_node->set_rendering_state(mrt<4>::rendering_mode::NORMALS);
    }
    
    if( key == GLFW_KEY_4 && action == GLFW_PRESS)
    {
        app.debug_node_3d->set_active(false);
        app.mrt_node->set_rendering_state(mrt<4>::rendering_mode::POSITIONS);
    }
    
    if( key == GLFW_KEY_5 && action == GLFW_PRESS)
    {
        app.debug_node_3d->set_active(false);
        app.mrt_node->set_rendering_state(mrt<4>::rendering_mode::DEPTH);
    }
    if( key == GLFW_KEY_6 && action == GLFW_PRESS)
    {
        app.debug_node_3d->set_active(true);
        
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
    
    glfwCreateWindowSurface(device._instance, window, nullptr, &surface);
    device.create_logical_device(surface);
    vk::material_store material_store;
    material_store.create(&device);
    

    vk::glfw_swapchain swapchain(&device, window, surface);
    

    vk::graph<1> graph(&device, material_store, swapchain);
    
    app.device = &device;
    
    //glfwSetCursorPos(window, width * .5f, height * .5f);

    app.swapchain = &swapchain;

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

    
    display_texture_2d<1> debug_node(&device, &swapchain, swapchain.get_vk_swap_extent().width, swapchain.get_vk_swap_extent().height, "mario.png");

    graph.add_child(debug_node);
    
    app.graph = &graph;
    
    first_person_controller user_controler( app.perspective_camera, window);
    first_person_controller  texture_3d_view_controller(app.three_d_texture_camera, window);
    
    app.user_controller = &user_controler;
    app.texture_3d_view_controller = &texture_3d_view_controller;
    
    app.user_controller->update();
    app.texture_3d_view_controller->update();
    
    vk::graph<4> voxel_cone_tracing(&device, material_store, swapchain);
    
    mrt<4> mrt_node(&device, &swapchain);
    
    
    vk::orthographic_camera directional_light_cam(5.0f, 5.0f, 10.0f);
    directional_light_cam.up = glm::vec3(0.0,  1.0f, 0.0f);
    directional_light_cam.position = glm::vec3(0.0f, .8f, -5.0f);
    directional_light_cam.forward = -directional_light_cam.position;
    directional_light_cam.update_view_matrix();
    
    vsm<4> vsm_node(&device, swapchain.get_vk_swap_extent().width,
                    swapchain.get_vk_swap_extent().height, directional_light_cam);
    
    vsm_node.set_camera(directional_light_cam);
    vsm_node.set_name("vsm node");
    
    mrt_node.set_name("mrt");
    
    app.mrt_node = &mrt_node;
    
    eastl::array<voxelize<4>, 3> voxelizers;
    
    constexpr float distance = 8.f;
    std::array<glm::vec3, 3> cam_positions = {  glm::vec3(0.0f, 0.0f, -distance),glm::vec3(0.0f, distance, 0.0f), glm::vec3(distance, 0.0f, 0.0f)};
    std::array<glm::vec3, 3> up_vectors = { glm::vec3 {0.0f, 1.0f, 0.0f}, glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)};
    std::array<const char*, 3>     names = { "z-axis", "y-axis", "x-axis"};
    
    vk::orthographic_camera vox_proj_cam(10.0f, 10.0f, 10.0f);
    vox_proj_cam.up = up_vectors[0];
    vox_proj_cam.position = cam_positions[0];
    vox_proj_cam.forward = -cam_positions[0];
    vox_proj_cam.update_view_matrix();
    
    for( int i = 0; i < voxelizers.size(); ++i)
    {
        //TODO: EVERY FRAME TICK, YOU MUST UPDATE THE LIGHT POSITION FOR EACH VOXELIZER
        voxelizers[i].set_proj_to_voxel_screen(vox_proj_cam.get_projection_matrix() * vox_proj_cam.view_matrix);
        
        voxelizers[i].set_device(&device);
        voxelizers[i].set_cam_params(cam_positions[i], up_vectors[i]);
        voxelizers[i].set_name(names[i]);
        voxelizers[i].set_dimensions(voxelize<4>::VOXEL_CUBE_WIDTH, voxelize<4>::VOXEL_CUBE_HEIGHT);
        
        //TODO: LET'S CREATE A MESH NODE, ATTACH THESE TO VOXELIZERS
        for(int j = 0; j < app.shapes.size(); ++j)
        {
            voxelizers[i].add_object(*(app.shapes[j]));
        }
    }
    
    for( int i = 0; i < app.shapes.size(); ++i)
    {
        mrt_node.add_object(*(app.shapes[i]));
        vsm_node.add_object(*(app.shapes[i]));
    }

    
    eastl::array<clear_3d_textures<4>, mip_map_3d_texture<4>::TOTAL_LODS> clear_mip_maps;
    eastl::array<mip_map_3d_texture<4>, mip_map_3d_texture<4>::TOTAL_LODS-1> three_d_mip_maps;


    clear_mip_maps[0].set_device(&device);
    clear_mip_maps[0].set_group_size(voxelize<4>::VOXEL_CUBE_WIDTH / vk::compute_pipeline<1>::LOCAL_GROUP_SIZE,
                                     voxelize<4>::VOXEL_CUBE_HEIGHT / vk::compute_pipeline<1>::LOCAL_GROUP_SIZE,
                                     voxelize<4>::VOXEL_CUBE_DEPTH / vk::compute_pipeline<1>::LOCAL_GROUP_SIZE);

    static eastl::array<eastl::fixed_string<char, 100>, mip_map_3d_texture<4>::TOTAL_LODS > albedo_names = {};
    static eastl::array<eastl::fixed_string<char, 100>, mip_map_3d_texture<4>::TOTAL_LODS > normal_names = {};
    
    albedo_names[0] = "voxel_albedos";
    normal_names[0] = "voxel_normals";
    
    
    eastl::array< eastl::fixed_string<char, 100>, 2 > input_tex;
    eastl::array< eastl::fixed_string<char, 100>, 2 > output_tex;
    
    input_tex[0] = "voxel_albedos4";
    input_tex[1] = "voxel_normals4";
    
    output_tex[0] = "voxel_albedos5";
    output_tex[1] = "voxel_normals5";
    
    clear_mip_maps[0].set_clear_texture(albedo_names[0], normal_names[0]);
    
    clear_mip_maps[0].set_name("clear mip map 0");
    three_d_mip_maps[0].set_name("three d mip map 0");
    
    
    for( int map_id = 1; map_id < clear_mip_maps.size(); ++map_id)
    {
        assert((voxelize<4>::VOXEL_CUBE_WIDTH >> map_id) % vk::compute_pipeline<1>::LOCAL_GROUP_SIZE == 0 && "invalid voxel cube size, voxel texture will not clear properly");
        assert((voxelize<4>::VOXEL_CUBE_HEIGHT >> map_id) % vk::compute_pipeline<1>::LOCAL_GROUP_SIZE == 0 && "invalid voxel cube size, voxel texture will not clear properly");
        assert((voxelize<4>::VOXEL_CUBE_DEPTH >> map_id) % vk::compute_pipeline<1>::LOCAL_GROUP_SIZE == 0 && "invalid voxel cube size, voxel texture will not clear properly");


        uint32_t local_groups_x = (voxelize<4>::VOXEL_CUBE_WIDTH >> map_id) / vk::compute_pipeline<1>::LOCAL_GROUP_SIZE;
        uint32_t local_groups_y = (voxelize<4>::VOXEL_CUBE_HEIGHT >> map_id) / vk::compute_pipeline<1>::LOCAL_GROUP_SIZE;
        uint32_t local_groups_z = (voxelize<4>::VOXEL_CUBE_DEPTH >> map_id) / vk::compute_pipeline<1>::LOCAL_GROUP_SIZE;

        albedo_names[map_id].sprintf("voxel_albedos%i", map_id);
        normal_names[map_id].sprintf("voxel_normals%i", map_id);
        
        input_tex[0] = albedo_names[map_id -1];
        input_tex[1] = normal_names[map_id -1];
        
        output_tex[0] = albedo_names[map_id];
        output_tex[1] = normal_names[map_id];
        
        three_d_mip_maps[map_id -1].set_textures(input_tex, output_tex);
        three_d_mip_maps[map_id -1].set_device(&device);
        three_d_mip_maps[map_id -1].set_group_size(local_groups_x,local_groups_y,local_groups_z);
        
        eastl::fixed_string<char, 100> name = {};
        
        name.sprintf( "three d mip map %i with local group %i", map_id, local_groups_z);
        three_d_mip_maps[map_id -1].set_name(name.c_str());
        
        //TODO: we also need to clear the normal voxel textures
        clear_mip_maps[map_id].set_clear_texture(albedo_names[map_id], normal_names[map_id]);
        clear_mip_maps[map_id].set_device(&device);
        clear_mip_maps[map_id].set_group_size(local_groups_x, local_groups_y, local_groups_z);

        name.sprintf("clear mip map node %i with local group %i", map_id, local_groups_x);
        clear_mip_maps[map_id].set_name( name.c_str()) ;

    }
    
    //build the graph!
    
    //attach mip map nodes together starting with the lowest mip map all the way up to the highest
    for( unsigned long i = three_d_mip_maps.size()-1 ; i > 0 ; --i)
    {
        three_d_mip_maps[i].add_child( three_d_mip_maps[i-1]);
    }
    
    //attach all clear maps to the last voxelizer
    for( int i = 0; i < clear_mip_maps.size(); ++i)
    {
        voxelizers[voxelizers.size() -1].add_child(clear_mip_maps[i]);
    }
    
    //chain voxelizers together
    for( int i = 0; i < voxelizers.size()-1; ++i)
    {
        voxelizers[i].add_child( voxelizers[i + 1] );
    }

    //attach the zero voxelizer to the highest three_d mip map...
    three_d_mip_maps[0].add_child(voxelizers[0]);

    glm::vec2 dims = {swapchain.get_vk_swap_extent().width, swapchain.get_vk_swap_extent().height };
    display_texture_3d<4> debug_node_3d(&device,&swapchain, dims, "voxel_albedos2" );

    debug_node_3d.set_name("3d-texture-render");
    debug_node_3d.set_active(false);
    
    debug_node_3d.set_3D_texture_cam(three_d_texture_cam);
    
    debug_node_3d.add_child( three_d_mip_maps[three_d_mip_maps.size()-1]);
    
    
    display_texture_2d<4> vsm_debug(&device, &swapchain, (uint32_t)dims.x, (uint32_t)dims.y, "vsm");
    vsm_debug.set_active(false);
    vsm_debug.set_name("vsm_debug");
    
    vsm_node.add_child(debug_node_3d);
    vsm_debug.add_child(vsm_node);
    
    //mrt_node.add_child(vsm_debug);
    

    gaussian_blur<4> gsb_vertical(&device,  dims.x, dims.y, gaussian_blur<4>::DIRECTION::VERTICAL, "vsm", "gauss_vertical");
    gaussian_blur<4> gsb_horizontal(&device, dims.x, dims.y, gaussian_blur<4>::DIRECTION::HORIZONTAL, "gauss_vertical", "blur_final");
    display_texture_2d<4> gsm_debug(&device, &swapchain, (uint32_t)dims.x, (uint32_t)dims.y, "blur_final");

    gsb_vertical.add_child(vsm_debug);
    gsb_horizontal.add_child(gsb_vertical);

    gsm_debug.add_child(gsb_horizontal);
    gsm_debug.set_active(true);
    //attach the mrt node to the graph
    voxel_cone_tracing.add_child(gsm_debug);

    //voxel_cone_tracing.add_child(mrt_node);
    app.voxel_graph = &voxel_cone_tracing;
    app.debug_node_3d = &debug_node_3d;
    
    app.voxel_graph->init();
    //app.graph->init();
    
    //game_loop_ortho();
    game_loop();
    
    device.wait_for_all_operations_to_finish();

    app.voxel_graph->destroy_all();
    //app.graph->destroy_all();
    
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
