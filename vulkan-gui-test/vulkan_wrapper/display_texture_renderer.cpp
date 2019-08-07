//
//  display_texture_renderer.cpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 8/6/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#include "display_2d_texture_renderer.h"


using namespace vk;

display_2d_texture_renderer::display_2d_texture_renderer(vk::device* device, GLFWwindow* window, swapchain* swapchain, material_store& store):
renderer(device, window, swapchain, store.GET_MAT<visual_material>("display")),
_render_plane(device)
{
    _render_plane.create();
}
