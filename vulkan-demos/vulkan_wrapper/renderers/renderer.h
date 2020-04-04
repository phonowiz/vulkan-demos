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
#include "material_store.h"


namespace vk
{
    class device;
    class glfw_swapchain;
    class depth_texture;
    class obj_shape;
    class texture_2d;
    
    template< uint32_t NUM_ATTACHMENTS>
    class renderer : public object
    {
        
    public:
        
        using render_pass_type =  render_pass< NUM_ATTACHMENTS>;
        using graphics_pipeline_type = typename render_pass< NUM_ATTACHMENTS>::graphics_pipeline_type;
        enum class subpass_layout
        {
            RENDER_TARGET_LAYOUT = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            READ_ONLY_LAYOUT = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            DEPTH_READ_ONLY_LAYOUT = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
            LAYOUT_UNDEFINED = VK_IMAGE_LAYOUT_UNDEFINED
        };

        
    public:
        
        renderer(){};
        renderer(device* device, GLFWwindow* window, glfw_swapchain* swapchain, material_store& store, const char* material_name);
        
        virtual void draw(camera &camera);
        
        graphics_pipeline_type& get_pipeline(uint32_t subpass_id) { return _render_pass.get_pipeline(_image_index, subpass_id ); }
        graphics_pipeline_type& get_pipeline(uint32_t swapchain_id, uint32_t subpass_id) { return _render_pass.get_subpass(subpass_id).get_pipeline( swapchain_id); }
        render_pass_type& get_render_pass(){ return _render_pass;}
        
        virtual void init();
        void recreate_renderer();
        
        virtual void destroy() override;
        
        inline shader_parameter::shader_params_group& get_uniform_params(uint32_t swapchain_id, uint32_t subpass_id, material_base::parameter_stage stage, uint32_t binding)
        {
            return _render_pass.get_pipeline(swapchain_id, subpass_id ).get_uniform_parameters(stage, binding);
        }
        
        inline shader_parameter::shader_params_group& get_uniform_params(uint32_t subpass_id, material_base::parameter_stage stage, uint32_t binding)
        {
            static int32_t index = -1;
            index = (index + 1) % glfw_swapchain::NUM_SWAPCHAIN_IMAGES;
            return _render_pass.get_pipeline(index, subpass_id ).get_uniform_parameters(stage, binding);
        }
        
        
        inline visual_material::object_shader_params_group& get_dynamic_params(uint32_t subpass_id, material_base::parameter_stage stage, uint32_t binding)
        {
            return _render_pass.get_pipeline(_image_index, subpass_id).get_dynamic_parameters(stage, binding);
        }
        
        void set_device(device* device)
        {
            _device = device;
            
            _render_pass.set_device(device);
        }
        void set_window(GLFWwindow* window)
        {
            _window = window;
        }
        void set_material( uint32_t subpass_id, visual_mat_shared_ptr material)
        {
            _render_pass.set_material(material);
        }
        
        
        void set_swapchain(glfw_swapchain* swapchain)
        {
            _swapchain = swapchain;
        }
        
        inline void set_next_swapchain_id(int32_t next_frame){ _next_frame = next_frame; }
        
        ~renderer();
        
    protected:
        virtual void create_command_buffers(VkCommandBuffer** command_buffers, VkCommandPool command_pool);
        virtual void create_semaphores_and_fences();
        
        void create_semaphore(VkSemaphore& semaphore);
        void create_semaphores(std::array<VkSemaphore, glfw_swapchain::NUM_SWAPCHAIN_IMAGES>& semaphores);
        void create_fence(VkFence& fence);
        virtual void record_command_buffers(VkCommandBuffer& command_buffer, uint32_t swapchain_id);
        virtual void perform_final_drawing_setup(VkCommandBuffer& buffer, uint32_t swapchain_id);
        
        
    private:
         uint32_t _image_index = 0;
    
    protected:
        
        device*             _device = nullptr;
        GLFWwindow*         _window = nullptr;
        
        VkSemaphore _semaphore_image_available[glfw_swapchain::NUM_SWAPCHAIN_IMAGES] = {};
        VkSemaphore _semaphore_rendering_done = VK_NULL_HANDLE;
        glfw_swapchain*  _swapchain = nullptr;
        int32_t _next_frame = 0;
        
        std::array<VkFence, glfw_swapchain::NUM_SWAPCHAIN_IMAGES> _composite_fence {};

        //TODO: find out what is the limit of attachments
        static const uint32_t MAX_ATTACHMENTS = 10;
        
        VkCommandBuffer* _command_buffers = nullptr;
        render_pass_type _render_pass;
        
        bool _pipeline_created[glfw_swapchain::NUM_SWAPCHAIN_IMAGES]= {};
    };

    #include "renderer.hpp"

}
