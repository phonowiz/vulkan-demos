//
//  present_texture.hpp
//  vulkan-demos
//
//  Created by Rafael Sabino on 1/1/20.
//  Copyright © 2020 Rafael Sabino. All rights reserved.
//

#pragma once


#include "image.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "EASTL/fixed_vector.h"

namespace vk
{
    class glfw_swapchain;

    class glfw_present_texture : public image
    {
    public:
        glfw_present_texture()
        {
            _original_layout = image_layouts::PRESENT_KHR;
        }
        inline void set_swapchain(glfw_swapchain* chain)
        {
            _swapchain = chain;
            _original_layout = image_layouts::PRESENT_KHR;
        }
        
        inline void set_swapchain_image_index (int32_t i){ _swapchain_image_index = i; }
        inline void set_window( GLFWwindow* window) { _window = window; }
        virtual void init() override;
        
        VkSurfaceFormatKHR get_vk_surface_format();
        VkPresentModeKHR get_vk_present_mode();
        
        virtual void create_sampler() override;
        virtual void create_image_view( VkImage image, VkFormat format, VkImageView& image_view) override;
        virtual void destroy() override;
        
        virtual char const * const * get_instance_type() override { return (&_image_type); };
        static char const * const * get_class_type(){ return (&_image_type); }
        
    private:
        
        glfw_swapchain* _swapchain = nullptr;
        GLFWwindow*   _window = nullptr;
        
        static constexpr int INVALID = -1;
        int32_t    _swapchain_image_index = INVALID;
        
        static constexpr char  const * _image_type = nullptr;
    };
}
