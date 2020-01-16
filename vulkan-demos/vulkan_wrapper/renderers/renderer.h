//
//  renderer.hpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 2/7/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#pragma once


#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>
#include "visual_material.h"
#include "depth_texture.h"
#include "graphics_pipeline.h"
#include "./cameras/camera.h"
#include "../renderers/render_pass.h"
#include "../textures/glfw_present_texture.h"


namespace vk
{
    class device;
    class glfw_swapchain;
    class depth_texture;
    class obj_shape;
    class texture_2d;
    
    template<typename RENDER_TEXTURE_TYPE, uint32_t NUM_ATTACHMENTS>
    class renderer : public object
    {
        
    public:
        enum class subpass_layout
        {
            RENDER_TARGET_LAYOUT = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            READ_ONLY_LAYOUT = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            DEPTH_READ_ONLY_LAYOUT = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
            LAYOUT_UNDEFINED = VK_IMAGE_LAYOUT_UNDEFINED
        };

        
    public:
        
        renderer(){};
        renderer(device* device, GLFWwindow* window, glfw_swapchain* swapchain, visual_mat_shared_ptr material);
        

        void add_shape(obj_shape* s){ _shapes.push_back(s); };
        
        virtual void draw(camera &camera);
        
        graphics_pipeline<RENDER_TEXTURE_TYPE, NUM_ATTACHMENTS>& get_pipeline() { return _pipeline;}
        
        virtual void init();
        void recreate_renderer();
        
        virtual void destroy() override;
        
        inline shader_parameter::shader_params_group& get_uniform_params(material_base::parameter_stage stage, uint32_t binding)
        {
            return _pipeline.get_uniform_parameters(stage, binding, _image_index);
        }
        
        inline visual_material::object_shader_params_group& get_dynamic_params(material_base::parameter_stage stage, uint32_t binding)
        {
            return _pipeline.get_dynamic_parameters(stage, binding, _image_index);
        }
        
        void set_device(device* device)
        {
            _device = device;
            _pipeline.set_device(device);
        }
        void set_window(GLFWwindow* window)
        {
            _window = window;
        }
        void set_material( visual_mat_shared_ptr material)
        {
            _pipeline.set_material(material);
        }
        
        
        void set_swapchain(glfw_swapchain* swapchain)
        {
            _swapchain = swapchain;
        }
        
        ~renderer();
        
        
    protected:
        virtual void create_command_buffers(VkCommandBuffer** command_buffers, VkCommandPool command_pool);
        virtual void create_semaphores_and_fences();
        
        void create_semaphore(VkSemaphore& semaphore);
        void create_fence(VkFence& fence);
        
        virtual void record_command_buffers(obj_shape** shapes, size_t number_of_shapes);
        virtual void perform_final_drawing_setup();
        
    private:
         uint32_t _image_index = 0;
    
    protected:
        
        device*             _device = nullptr;
        GLFWwindow*         _window = nullptr;
        
        VkSemaphore _semaphore_image_available = VK_NULL_HANDLE;
        VkSemaphore _semaphore_rendering_done = VK_NULL_HANDLE;
        glfw_swapchain*  _swapchain = nullptr;
        
        std::array<VkFence, glfw_swapchain::NUM_SWAPCHAIN_IMAGES> _composite_fence {};

        
        //TODO: find out what is the limit of attachments
        static const uint32_t MAX_ATTACHMENTS = 10;
        
        VkCommandBuffer* _command_buffers = nullptr;
        graphics_pipeline<RENDER_TEXTURE_TYPE, NUM_ATTACHMENTS> _pipeline;
        
        std::vector<obj_shape*> _shapes;
        
        bool _pipeline_created = false;
    };

    #include "renderer.hpp"

}
