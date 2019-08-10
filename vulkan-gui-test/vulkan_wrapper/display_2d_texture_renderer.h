//
//  display_texture_renderer.hpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 8/6/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "renderer.h"
#include "material_store.h"
#include "./shapes/screen_plane.h"
#include "device.h"
#include "texture_2d.h"

namespace vk
{
    class display_2d_texture_renderer : public renderer
    {
    public:
        display_2d_texture_renderer(vk::device* device, GLFWwindow* window, swapchain* swapchain, material_store& store);
        
        void show_texture(texture_2d* texture);
        
        virtual void init() override;
        
        virtual void destroy() override;
    
    private:
        screen_plane _render_plane;
    
    };
}
