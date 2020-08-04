
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

#include "vulkan_wrapper/render_graph/assimp_node.h"

#include "vulkan_wrapper/cameras/perspective_camera.h"
#include "camera_controllers/first_person_controller.h"
#include "camera_controllers/circle_controller.h"

#include "graph_nodes/graphics_nodes/display_texture_2d.h"
#include "graph_nodes/graphics_nodes/display_texture_3d.h"
#include "graph_nodes/graphics_nodes/vsm.h"
#include "graph_nodes/graphics_nodes/gaussian_blur.h"
#include "graph_nodes/graphics_nodes/pbr.h"
#include "graph_nodes/graphics_nodes/fxaa.h"
#include "graph_nodes/graphics_nodes/luminance.h"

#include "graph_nodes/compute_nodes/mip_map_3d_texture.hpp"
#include "graph_nodes/graphics_nodes/voxelize.h"
#include "graph_nodes/compute_nodes/clear_3d_texture.hpp"
#include "graph_nodes/graphics_nodes/mrt.h"
#include "graph_nodes/graphics_nodes/atmospheric.h"


#include "new_operators.h"
#include "graph.h"

#include <filesystem>

namespace fs = std::filesystem;



///an excellent summary of vulkan can be found here:
//https://renderdoc.org/vulkan-in-30-minutes.html


GLFWwindow   *window = nullptr;
VkSurfaceKHR surface;
VkSurfaceKHR surface2;

int width = 1024;
int height = 768;

void start_glfw() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    //my computer cannot handle retina right now, commented for this reason
    //glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_TRUE);
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

enum class camera_type
{
    USER,
    DEMO,
    THREE_D_TEXTURE
};
struct App
{
    vk::device* device = nullptr;

    vk::graph<1> * graph = nullptr;

    vk::graph<4> * voxel_graph = nullptr;
    eastl::shared_ptr<mrt<4>> mrt_node = nullptr;
    eastl::shared_ptr<display_texture_3d<4>> debug_node_3d = nullptr;

    first_person_controller* user_controller = nullptr;
    first_person_controller* texture_3d_view_controller = nullptr;
    circle_controller* circle_controller = nullptr;

    vk::camera*     perspective_camera = nullptr;
    vk::camera*     three_d_texture_camera = nullptr;
    vk::glfw_swapchain*  swapchain = nullptr;
    vk::material_store* material_store = nullptr;

    std::vector<vk::obj_shape*> shapes;
    std::vector<vk::obj_shape*> shapes_lods;
    fxaa<4>* aa = nullptr;
    display_texture_2d<4>* debug = nullptr;
    

    camera_type cam_type = camera_type::USER;
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

        if(app.cam_type == camera_type::USER)
        {
            app.user_controller->update();
        }
        else if( app.cam_type == camera_type::THREE_D_TEXTURE)
        {
            app.texture_3d_view_controller->update();
        }
        else if( app.cam_type == camera_type::DEMO)
            app.circle_controller->update();


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
        app.cam_type = camera_type::USER;
        app.debug_node_3d->set_active(false);
        app.mrt_node->set_rendering_state(mrt<4>::rendering_mode::FULL_RENDERING);
    }

    if (key == GLFW_KEY_2 && action == GLFW_PRESS)
    {
        app.cam_type = camera_type::USER;
        app.debug_node_3d->set_active(false);
        app.mrt_node->set_rendering_state(mrt<4>::rendering_mode::ALBEDO);
    }

    if (key == GLFW_KEY_3 && action == GLFW_PRESS)
    {
        app.cam_type = camera_type::USER;
        app.debug_node_3d->set_active(false);
        app.mrt_node->set_rendering_state(mrt<4>::rendering_mode::NORMALS);
    }

    if( key == GLFW_KEY_4 && action == GLFW_PRESS)
    {
        app.cam_type = camera_type::USER;
        app.debug_node_3d->set_active(false);
        app.mrt_node->set_rendering_state(mrt<4>::rendering_mode::POSITIONS);
    }

    if( key == GLFW_KEY_5 && action == GLFW_PRESS)
    {
        app.cam_type = camera_type::USER;
        app.debug_node_3d->set_active(false);
        app.mrt_node->set_rendering_state(mrt<4>::rendering_mode::DEPTH);
    }
    if( key == GLFW_KEY_6 && action == GLFW_PRESS)
    {

        app.debug_node_3d->set_active(true);
        app.cam_type = camera_type::THREE_D_TEXTURE;
        app.texture_3d_view_controller->reset();
    }

    if( key == GLFW_KEY_7 && action == GLFW_PRESS)
    {
        app.cam_type = camera_type::USER;
        app.debug_node_3d->set_active(false);
        app.mrt_node->set_rendering_state(mrt<4>::rendering_mode::VARIANCE_SHADOW_MAP);
    }
    if( key == GLFW_KEY_8 && action == GLFW_PRESS)
    {
        app.cam_type = camera_type::USER;
        app.debug_node_3d->set_active(false);
        app.mrt_node->set_rendering_state(mrt<4>::rendering_mode::AMBIENT_LIGHT);
    }
    if( key == GLFW_KEY_9 && action == GLFW_PRESS)
    {
        app.cam_type = camera_type::USER;
        app.debug_node_3d->set_active(false);
        app.mrt_node->set_rendering_state(mrt<4>::rendering_mode::AMBIENT_OCCLUSION);
    }
    if( key == GLFW_KEY_0 && action == GLFW_PRESS)
    {
        app.cam_type = camera_type::USER;
        app.debug_node_3d->set_active(false);
        app.mrt_node->set_rendering_state(mrt<4>::rendering_mode::DIRECT_LIGHT);
    }
    if( key == GLFW_KEY_L && action == GLFW_PRESS)
    {
        static bool user_cam_locked = false;
        user_cam_locked = !user_cam_locked;
        app.user_controller->lock(user_cam_locked);
        app.user_controller->reset();
    }
    if( key == GLFW_KEY_M && action == GLFW_PRESS)
    {
        static bool aa_on = true;
        app.aa->set_active(!aa_on);
        app.debug->set_active(aa_on);
        aa_on = !aa_on;
        app.debug_node_3d->set_active(false);
    }
    
    if( key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        app.quit = true;
    }


}
void create_graph()
{
    
    //eastl::shared_ptr<vk::assimp_node<4>> cornell_node = eastl::make_shared<vk::assimp_node<4>>(app.device, "cornell/cornell_box.obj");
    eastl::shared_ptr<vk::assimp_node<4>> floor = eastl::make_shared<vk::assimp_node<4>>(app.device, "plane/plane.fbx");
    //eastl::shared_ptr<vk::assimp_node<4>> model_node = eastl::make_shared<vk::assimp_node<4>>( app.device, "IndoorPotPlant/indoor_plant_02_fbx/indoor_plant_02_6.1_1+2_tri.fbx" );
    
    floor->set_texture_relative_path("Ground037_2K_Color.png", aiTextureType_BASE_COLOR);
    floor->set_texture_relative_path("../../textures/black.png", aiTextureType_METALNESS);
    floor->set_texture_relative_path("Ground037_2K_Normal.png", aiTextureType_NORMAL_CAMERA);
    floor->set_texture_relative_path("../../textures/white.png", aiTextureType_DIFFUSE_ROUGHNESS);
    floor->set_texture_relative_path("Ground037_2K_AmbientOcclusion.png", aiTextureType_AMBIENT_OCCLUSION);
    
    eastl::shared_ptr<vk::assimp_node<4>> model_node =
            eastl::make_shared<vk::assimp_node<4>>( app.device, "1977-plymouth-volaire-sedan/source/549152622d66472dae9489efa29991c4.rar/plymouthfix-modified.fbx" );
    
    model_node->set_texture_relative_path("Material_65_Base_Color.png", aiTextureType_BASE_COLOR);
    model_node->set_texture_relative_path("Material_65_Metallic.png", aiTextureType_METALNESS);
    model_node->set_texture_relative_path("Material_65_Normal_DirectX.png", aiTextureType_NORMAL_CAMERA);
    model_node->set_texture_relative_path("Material_65_Roughness.png", aiTextureType_DIFFUSE_ROUGHNESS);
    model_node->set_texture_relative_path("Material_65_Mixed_AO.png", aiTextureType_AMBIENT_OCCLUSION);
    
    
    vk::transform trans ={};

    trans.position = glm::vec3(-0.4f, -0.00f, -.6f);
    //these things are necessary because assimp won't pick up on scaling/rotation attributes specified in the Blender exporter
    //I don't know how to fix these things from the blender or assimp side.
    trans.scale = glm::vec3(.0004f,.0004f, .0004f);
    trans.rotation.x = glm::half_pi<float>();
    trans.rotation.y = .9f;
    trans.update_transform_matrix();


    model_node->init_transforms(trans);
    trans = {};
    trans.rotation.x =  1.5708f;
    trans.position = glm::vec3(0.0f, -.00f, 0.0f);
    trans.scale = glm::vec3(2.5f, 2.5f, 2.5f);

    trans.update_transform_matrix();

    floor->init_transforms(trans);

    float aspect = static_cast<float>(app.swapchain->get_vk_swap_extent().width)/ static_cast<float>(app.swapchain->get_vk_swap_extent().height);
    vk::perspective_camera perspective_camera(glm::radians(45.0f),
                                              aspect, .01f, 100.0f);

    vk::perspective_camera three_d_texture_cam(glm::radians(45.0f),
                                               aspect, .01f, 50.0f);

    app.perspective_camera = &perspective_camera;
    app.perspective_camera->position = glm::vec3(0.0f, 1.0f, -5.0f);
    app.perspective_camera->forward = -perspective_camera.position;


    glm::vec3 eye(0.0f, 0.0f, -3.0f);
    app.three_d_texture_camera = &three_d_texture_cam;
    app.three_d_texture_camera->position = eye;
    app.three_d_texture_camera->forward = -eye;

    first_person_controller user_controler( app.perspective_camera, window);
    first_person_controller  texture_3d_view_controller(app.three_d_texture_camera, window);
    circle_controller   circle_controller(app.perspective_camera, 1.8f, glm::vec3(0.0f, -.02f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));

    app.user_controller = &user_controler;
    app.circle_controller = &circle_controller;
    app.texture_3d_view_controller = &texture_3d_view_controller;

    app.user_controller->update();
    app.texture_3d_view_controller->update();

    vk::graph<4> voxel_cone_tracing(app.device, *app.material_store, *app.swapchain);

    vk::perspective_camera point_light_cam(glm::radians(110.0f),
                                                  aspect, .01f, 10.0f);

    point_light_cam.up = glm::vec3(0.0,  1.0f, 0.001f);
    point_light_cam.position = glm::vec3(0.f, 2.f, -0.8f);
    point_light_cam.forward = -point_light_cam.position;
    point_light_cam.update_view_matrix();

    eastl::shared_ptr<mrt<4>> mrt_node = eastl::make_shared<mrt<4>>(app.device, app.swapchain, point_light_cam, mrt<4>::light_type::POINT_LIGHT);

    eastl::shared_ptr<vsm<4>> vsm_node = eastl::make_shared<vsm<4>>(app.device, app.swapchain->get_vk_swap_extent().width,
                    app.swapchain->get_vk_swap_extent().height, point_light_cam);

    vsm_node->set_name("vsm node");

    mrt_node->set_name("mrt");
    mrt_node->set_rendering_state( mrt<4>::rendering_mode::FULL_RENDERING);
    app.mrt_node = mrt_node;

    eastl::vector<eastl::shared_ptr<voxelize<4>>> voxelizers;
    for( int i = 0; i < 3; ++i)
    {
        voxelizers.push_back( eastl::make_shared<voxelize<4>>());
    }

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
        voxelizers[i]->set_proj_to_voxel_screen(vox_proj_cam.get_projection_matrix() * vox_proj_cam.view_matrix);

        voxelizers[i]->set_device(app.device);
        voxelizers[i]->set_cam_params(cam_positions[i], up_vectors[i]);
        voxelizers[i]->set_name(names[i]);
        voxelizers[i]->set_dimensions(voxelize<4>::VOXEL_CUBE_WIDTH, voxelize<4>::VOXEL_CUBE_HEIGHT);

        voxelizers[i]->set_key_light_cam(point_light_cam, voxelize<4>::light_type::POINT_LIGHT);

        voxelizers[i]->add_child(*model_node);
        voxelizers[i]->add_child(*floor);
    }

    mrt_node->add_child(*model_node);
    mrt_node->add_child(*floor);

    vsm_node->add_child(*model_node);
    vsm_node->add_child(*floor);


    eastl::array<clear_3d_textures<4>, mip_map_3d_texture<4>::TOTAL_LODS> clear_mip_maps;
    eastl::array<mip_map_3d_texture<4>, mip_map_3d_texture<4>::TOTAL_LODS-1> three_d_mip_maps;


    clear_mip_maps[0].set_device(app.device);
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
        EA_ASSERT_MSG((voxelize<4>::VOXEL_CUBE_WIDTH >> map_id) % vk::compute_pipeline<1>::LOCAL_GROUP_SIZE == 0, "invalid voxel cube size, voxel texture will not clear properly");
        EA_ASSERT_MSG((voxelize<4>::VOXEL_CUBE_HEIGHT >> map_id) % vk::compute_pipeline<1>::LOCAL_GROUP_SIZE == 0, "invalid voxel cube size, voxel texture will not clear properly");
        EA_ASSERT_MSG((voxelize<4>::VOXEL_CUBE_DEPTH >> map_id) % vk::compute_pipeline<1>::LOCAL_GROUP_SIZE == 0, "invalid voxel cube size, voxel texture will not clear properly");


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
        three_d_mip_maps[map_id -1].set_device(app.device);
        three_d_mip_maps[map_id -1].set_group_size(local_groups_x,local_groups_y,local_groups_z);

        eastl::fixed_string<char, 100> name = {};

        name.sprintf( "three d mip map %i with local group %i", map_id, local_groups_z);
        three_d_mip_maps[map_id -1].set_name(name.c_str());

        //TODO: we also need to clear the normal voxel textures
        clear_mip_maps[map_id].set_clear_texture(albedo_names[map_id], normal_names[map_id]);
        clear_mip_maps[map_id].set_device(app.device);
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
        voxelizers[voxelizers.size() -1]->add_child(clear_mip_maps[i]);
    }

    //chain voxelizers together
    for( int i = 0; i < voxelizers.size()-1; ++i)
    {
        voxelizers[i]->add_child( *voxelizers[i + 1] );
    }

    //attach the zero voxelizer to the highest three_d mip map...
    three_d_mip_maps[0].add_child(*voxelizers[0]);

    glm::vec2 dims = {app.swapchain->get_vk_swap_extent().width, app.swapchain->get_vk_swap_extent().height };
    eastl::shared_ptr<display_texture_3d<4>> debug_node_3d = eastl::make_shared<display_texture_3d<4>>(app.device,app.swapchain, dims, "voxel_albedos" );

    debug_node_3d->set_name("3d-texture-render");
    debug_node_3d->set_active(false);

    debug_node_3d->set_3D_texture_cam(three_d_texture_cam);

    debug_node_3d->add_child( three_d_mip_maps[three_d_mip_maps.size()-1]);
    vsm_node->add_child(*debug_node_3d);


    eastl::shared_ptr<gaussian_blur<4>> gsb_vertical = eastl::make_shared<gaussian_blur<4>> (app.device,  dims.x, dims.y, gaussian_blur<4>::DIRECTION::VERTICAL, "vsm", "gauss_vertical");
    eastl::shared_ptr<gaussian_blur<4>> gsb_horizontal = eastl::make_shared<gaussian_blur<4>>(app.device, dims.x, dims.y, gaussian_blur<4>::DIRECTION::HORIZONTAL, "gauss_vertical", "blur_final");

    gsb_vertical->add_child(*vsm_node);
    gsb_horizontal->add_child(*gsb_vertical);


    eastl::shared_ptr<pbr<4>> pbr_node = eastl::make_shared<pbr<4>>(app.device, dims.x, dims.y);
    
    pbr_node->add_child(*gsb_horizontal);
    
    pbr_node->add_child(*model_node);
    pbr_node->add_child(*floor);

    
    pbr_node->set_name("pbr node");
    //pbr_node->set_active(false);
    
    eastl::shared_ptr<fxaa<4>> fast_approximate_aa = eastl::make_shared<fxaa<4>>(app.device, app.swapchain,"atmospheric");
    mrt_node->add_child(*pbr_node);
    
    fast_approximate_aa->set_name("fxaa");
    
    eastl::shared_ptr<display_texture_2d<4>> pbr_debug = eastl::make_shared<display_texture_2d<4>>(app.device, app.swapchain, (uint32_t)dims.x, (uint32_t)dims.y, "normals");
    //eastl::shared_ptr<display_texture_2d<4>> pbr_debug = eastl::make_shared<display_texture_2d<4>>(app.device, app.swapchain, (uint32_t)dims.x, (uint32_t)dims.y, "model_albedo", vk::texture_2d::get_class_type());
    
    eastl::shared_ptr<atmospheric<4>> atmos_node = eastl::make_shared<atmospheric<4>>(app.device, app.swapchain);
    
    atmos_node->add_child(*mrt_node);
    fast_approximate_aa->add_child(*atmos_node);
    fast_approximate_aa->set_active(true);
    
    pbr_debug->add_child(*fast_approximate_aa);
    pbr_debug->set_name("pbr debug");
    pbr_debug->set_active(false);
    
    voxel_cone_tracing.add_child(*pbr_debug);
    app.voxel_graph = &voxel_cone_tracing;
    app.debug_node_3d = debug_node_3d;

    app.voxel_graph->init();
    
    app.aa = fast_approximate_aa.get();
    app.debug = pbr_debug.get();

    game_loop();

    app.device->wait_for_all_operations_to_finish();
    app.voxel_graph->destroy_all();

    voxelizers.clear();
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
    app.device = &device;
    //glfwSetCursorPos(window, width * .5f, height * .5f);

    app.swapchain = &swapchain;

    create_graph();

    material_store.destroy();
    swapchain.destroy();

    vkDestroySurfaceKHR(device._instance, surface, nullptr);
    device.destroy();

    shutdown_glfw();
    return 0;
}
