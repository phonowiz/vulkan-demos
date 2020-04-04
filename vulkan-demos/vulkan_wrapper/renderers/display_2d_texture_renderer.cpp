//
//  display_texture_renderer.cpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 8/6/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#include "display_2d_texture_renderer.h"


using namespace vk;

display_2d_texture_renderer::display_2d_texture_renderer(vk::device* device, GLFWwindow* window, glfw_swapchain* swapchain, material_store& store):
renderer(device, window, swapchain, store, "display"),
_render_plane(device)
{
    _render_pass.add_object(_render_plane);
    _render_plane.create();
}

void display_2d_texture_renderer::show_texture(texture_2d *texture)
{
    _render_pass.get_subpass(0).set_image_sampler(*texture, "tex", vk::material_base::parameter_stage::FRAGMENT, 1,
                                   vk::visual_material::usage_type::COMBINED_IMAGE_SAMPLER);
}

void display_2d_texture_renderer::init()
{
    renderer::init();
}

void display_2d_texture_renderer::destroy()
{
    _render_plane.destroy();
    renderer::destroy();
}
