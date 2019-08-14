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
#include "./shapes/screen_plane.h"
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
        
        
        //note: these are tied to deferred_output.frag values, if these values change, then change shader accordingly
        enum class rendering_state
        {
            ALBEDO = 0,
            NORMALS,
            POSITIONS,
            DEPTH,
            FULL_RENDERING
        };
        
        virtual vk::visual_mat_shared_ptr &  get_material() override { return _mrt_material; }
        
        inline texture_3d* get_voxel_texture(){ return &_voxel_3d_texture;}
        
        
        inline texture_2d* get_voxelizer_cam_texture( ){ return &_voxel_2d_view; }
        inline void set_rendering_state( rendering_state state ){ _rendering_state = state; }
        inline rendering_state get_rendering_state() { return _rendering_state; }
        
        
        void wait_for_all_fences();
        virtual void destroy() override;
        virtual void draw(camera& camera) override;

        
    private:
        virtual void create_render_pass()   override;
        virtual void create_frame_buffers() override;
        virtual void create_pipeline()      override;
        virtual void create_semaphores_and_fences() override;
        virtual void destroy_framebuffers() override;
        VkSemaphore generate_voxel_texture(vk::camera& camera);
        
        void compute(VkCommandBuffer command_buffer, vk::compute_pipeline& pipeline);
        void record_command_buffers(obj_shape** shapes, size_t number_of_shapes) override;
        void record_voxelize_command_buffers(obj_shape** shapes, size_t number_of_shapes);
        void record_clear_texture_3d_buffer ();
        
        void create_voxelization_render_pass();
        
        virtual void perform_final_drawing_setup() override;
        
        VkSampler       _color_sampler = VK_NULL_HANDLE;
        VkCommandBuffer *_offscreen_command_buffers = VK_NULL_HANDLE;
        VkCommandBuffer *_voxelize_command_buffers = VK_NULL_HANDLE;
        VkCommandBuffer *_clear_3d_texture_command_buffers = VK_NULL_HANDLE;
        
        graphics_pipeline _mrt_pipeline;
        graphics_pipeline _voxelize_pipeline;
        compute_pipeline  _clear_texture_3d_pipeline;
        
        VkRenderPass        _mrt_render_pass = VK_NULL_HANDLE;
        VkRenderPass        _voxelization_render_pass = VK_NULL_HANDLE;
        
        VkSemaphore _deferred_semaphore_image_available = VK_NULL_HANDLE;
        VkSemaphore _g_buffers_rendering_done = VK_NULL_HANDLE;
        VkSemaphore _voxelize_semaphore = VK_NULL_HANDLE;
        VkSemaphore _voxelize_semaphore_done = VK_NULL_HANDLE;
        VkSemaphore _clear_voxel_cube_semaphore = VK_NULL_HANDLE;
        VkSemaphore _clear_voxel_cube_smaphonre_done = VK_NULL_HANDLE;
        VkSemaphore _generate_voxel_x_axis_semaphore = VK_NULL_HANDLE;
        VkSemaphore _generate_voxel_y_axis_semaphore = VK_NULL_HANDLE;
        VkSemaphore _generate_voxel_z_axis_semaphore = VK_NULL_HANDLE;

        std::vector<VkFramebuffer>  _deferred_swapchain_frame_buffers;
        std::vector<VkFramebuffer>  _voxelize_frame_buffers;
        rendering_state _rendering_state = rendering_state::FULL_RENDERING;
        
        uint32_t _deferred_image_index = 0;
        
        orthographic_camera _ortho_camera;
        screen_plane        _screen_plane;
        
        glm::vec4 _light_pos = glm::vec4(.5f, .5f, 0.0f, 1.0f);
        glm::vec4 _light_color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        
        //TODO: on my mid 2014 macbook pro, the number of frames is 2, this could change in other platforms
        static constexpr uint32_t NUM_OF_FRAMES = 2;
        
        std::array<VkFence, NUM_OF_FRAMES> _g_buffers_fence {};
        std::array<VkFence, NUM_OF_FRAMES> _voxelize_inflight_fence {};
        std::array<VkFence, NUM_OF_FRAMES> _clear_voxel_texture_fence {};
        
        
        static constexpr uint32_t VOXEL_CUBE_WIDTH = 256u;
        static constexpr uint32_t VOXEL_CUBE_HEIGHT = 256u;
        static constexpr uint32_t VOXEL_CUBE_DEPTH  = 256u;
        bool _setup_initialized = false;

        
    };
}
