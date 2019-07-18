//
//  deferred_renderer.hpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 4/17/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#pragma once

#include "renderer.h"
#include "render_texture.h"
#include "graphics_pipeline.h"
#include "compute_pipeline.h"
#include "material_store.h"
#include "display_plane.h"
#include "depth_image.h"

namespace vk
{
    class swapchain;
    class image;
    
    class deferred_renderer : public renderer
    {
        
    private:
        struct attachment
        {
            image* texture = nullptr;
            const char* shader_parameter = nullptr;
            visual_material::parameter_stage stage = visual_material::parameter_stage::NONE;
            renderer::subpass_layout subpass_layout = subpass_layout::LAYOUT_UNDEFINED;
        };
        
        render_texture _positions;
        render_texture _normals;
        render_texture _albedo;
        depth_image    _depth;
        
    public:
        deferred_renderer(device* device, GLFWwindow* window, swapchain* swapchain, material_store& store);
        
        virtual vk::visual_mat_shared_ptr &  get_material() override { return _mrt_material; }
        
        virtual void destroy() override;

        
    private:
        virtual void create_render_pass()   override;
        virtual void create_frame_buffers() override;
        virtual void create_pipeline()      override;
        virtual void create_semaphores_and_fences() override;
        virtual void destroy_framebuffers() override;
        virtual void draw() override;
        
        void compute(VkCommandBuffer command_buffer, vk::compute_pipeline& pipeline);
        void record_command_buffers(mesh* meshes, size_t number_of_meshes) override;
        void record_voxelize_command_buffers();
        
        virtual void perform_final_drawing_setup() override;
        
        VkSampler       _color_sampler = VK_NULL_HANDLE;
        VkCommandBuffer *_offscreen_command_buffers = nullptr;
        VkCommandBuffer *_voxelize_command_buffers = VK_NULL_HANDLE;
        
        graphics_pipeline _mrt_pipeline;
        graphics_pipeline _debug_pipeline;
        compute_pipeline  _voxelize_pipeline;
        
        visual_mat_shared_ptr _mrt_material = nullptr;
        visual_mat_shared_ptr _debug_material = nullptr;
        
        VkRenderPass        _mrt_render_pass = VK_NULL_HANDLE;
        
        VkSemaphore _deferred_semaphore_image_available = VK_NULL_HANDLE;
        VkSemaphore _deferred_semaphore_rendering_done = VK_NULL_HANDLE;
        VkSemaphore _voxelize_semaphore = VK_NULL_HANDLE;
        VkSemaphore _voxelize_semaphore_done = VK_NULL_HANDLE;
        
        std::vector<VkFramebuffer>  _deferred_swapchain_frame_buffers;
        
        display_plane _plane;
        
        std::array<VkFence, 2> _deferred_inflight_fences {};
        std::array<VkFence, 2> _voxelize_inflight_fence {};
        
    };
}
