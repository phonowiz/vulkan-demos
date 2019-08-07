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
#include "cameras/orthographic_camera.h"

namespace vk
{
    class swapchain;
    class image;
    
    class deferred_renderer : public renderer
    {
        
    private:
        
        render_texture _positions;
        render_texture _normals;
        render_texture _albedo;
        depth_image    _depth;
        
        render_texture _voxel_2d_view;
        texture_3d _voxel_3d_texture;
        
        visual_mat_shared_ptr _mrt_material = nullptr;
        visual_mat_shared_ptr _debug_material = nullptr;
        
    public:
        deferred_renderer(device* device, GLFWwindow* window, swapchain* swapchain, material_store& store);
        
        virtual vk::visual_mat_shared_ptr &  get_material() override { return _mrt_material; }
        
        texture_3d* get_voxel_texture(){ return &_voxel_3d_texture; }
        texture_2d* get_voxelizer_cam_texture( ){return &_voxel_2d_view; }
        
        void wait_for_all_fences();
        virtual void destroy() override;
        virtual void draw(camera& camera) override;

        
    private:
        virtual void create_render_pass()   override;
        virtual void create_frame_buffers() override;
        virtual void create_pipeline()      override;
        virtual void create_semaphores_and_fences() override;
        virtual void destroy_framebuffers() override;

        
        void compute(VkCommandBuffer command_buffer, vk::compute_pipeline& pipeline);
        void record_command_buffers(mesh* meshes, size_t number_of_meshes) override;
        void record_voxelize_command_buffers(mesh* meshes, size_t number_of_meshes);
        void create_voxelization_render_pass();
        
        virtual void perform_final_drawing_setup() override;
        
        VkSampler       _color_sampler = VK_NULL_HANDLE;
        VkCommandBuffer *_offscreen_command_buffers = nullptr;
        VkCommandBuffer *_voxelize_command_buffers = VK_NULL_HANDLE;
        
        graphics_pipeline _mrt_pipeline;
        graphics_pipeline _voxelize_pipeline;
        
        VkRenderPass        _mrt_render_pass = VK_NULL_HANDLE;
        VkRenderPass        _voxelization_render_pass = VK_NULL_HANDLE;
        
        VkSemaphore _deferred_semaphore_image_available = VK_NULL_HANDLE;
        VkSemaphore _deferred_semaphore_rendering_done = VK_NULL_HANDLE;
        VkSemaphore _voxelize_semaphore = VK_NULL_HANDLE;
        VkSemaphore _voxelize_semaphore_done = VK_NULL_HANDLE;
        
        std::vector<VkFramebuffer>  _deferred_swapchain_frame_buffers;
        std::vector<VkFramebuffer>  _voxelize_frame_buffers;
        
        orthographic_camera _ortho_camera;
        
        display_plane _plane;
        
        std::array<VkFence, 2> _deferred_inflight_fences {};
        std::array<VkFence, 2> _voxelize_inflight_fence {};
        
        
        static constexpr uint32_t VOXEL_CUBE_WIDTH = 128u;
        static constexpr uint32_t VOXEL_CUBE_HEIGHT = 128u;
        static constexpr uint32_t VOXEL_CUBE_DEPTH  = 128u;
        bool _setup_initialized = false;

        
    };
}
