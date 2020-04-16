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
#include "depth_texture.h"
#include "cameras/orthographic_camera.h"
#include "glfw_swapchain.h"
#include "attachment_group.h"

namespace vk
{
    class glfw_swapchain;
    class image;
    
    class deferred_renderer : public renderer< 1>
    {
    public:
        deferred_renderer(device* device, GLFWwindow* window, glfw_swapchain* swapchain, material_store& store, std::vector<obj_shape*>& _shapes);
        
        
        //note: these are tied to deferred_output.frag values, if these values change, then change shader accordingly
        enum class rendering_mode
        {
            ALBEDO = 0,
            NORMALS,
            POSITIONS,
            DEPTH,
            FULL_RENDERING,
            AMBIENT_OCCLUSION,
            AMBIENT_LIGHT,
            DIRECT_LIGHT
        };
        
        inline eastl::array<texture_3d, glfw_swapchain::NUM_SWAPCHAIN_IMAGES>& get_voxel_texture( )
        {
            return _voxel_albedo_textures[2];
        }
        
        inline texture_2d* get_voxelizer_cam_texture( ){ return  &_voxel_2d_view[0][0]; }
        inline void set_rendering_state( rendering_mode state ){ _rendering_mode = state; }
        inline rendering_mode get_rendering_state() { return _rendering_mode; }
        
        
        void wait_for_all_fences();
        virtual void destroy() override;
        virtual void draw(camera& camera) override;
        
        static constexpr uint32_t VOXEL_CUBE_WIDTH = 256u;
        static constexpr uint32_t VOXEL_CUBE_HEIGHT = 256u;
        static constexpr uint32_t VOXEL_CUBE_DEPTH  =  256u ;
        
    private:

        virtual void create_semaphores_and_fences() override;
        void generate_voxel_textures(vk::camera& camera);
        
        void compute(VkCommandBuffer command_buffer, vk::compute_pipeline<1>& pipeline);
        virtual void record_command_buffers(VkCommandBuffer& buffer, uint32_t swapchain_id) override;
        void record_voxelize_command_buffers(VkCommandBuffer& buffer, uint32_t swapchain_id);
        void record_clear_texture_3d_buffer ( uint32_t swapchain_id);
        void record_3d_mip_maps_commands(uint32_t swapchain_id);

        void clear_voxels_textures();
        void generate_voxel_mip_maps();
        
        void create_voxel_texture_pipelines(vk::material_store& store);
        void setup_sampling_rays();
        
        virtual void perform_final_drawing_setup(VkCommandBuffer& buffer, uint32_t swapchain_id) override;
        
        VkSampler       _color_sampler = VK_NULL_HANDLE;
        VkCommandBuffer *_offscreen_command_buffers = VK_NULL_HANDLE;
        VkCommandBuffer *_voxelize_command_buffers = VK_NULL_HANDLE;
        
        using mrt_render_pass = render_pass<4>;
        mrt_render_pass _mrt_render_pass;
        
        using voxelize_render_pass = render_pass<1>;
        voxelize_render_pass _voxelize_render_pass;
  
        static constexpr unsigned int TOTAL_LODS = 6;
        
        eastl::array<eastl::array<compute_pipeline<1>, glfw_swapchain::NUM_SWAPCHAIN_IMAGES>, TOTAL_LODS> _clear_voxel_texture_pipeline {};
        
        eastl::array<VkCommandBuffer*, TOTAL_LODS> _clear_3d_texture_command_buffers {};
        eastl::array<VkCommandBuffer*, TOTAL_LODS -1> _genered_3d_mip_maps_commands {};
        eastl::array<eastl::array<compute_pipeline<1>, glfw_swapchain::NUM_SWAPCHAIN_IMAGES>, TOTAL_LODS -1> _create_voxel_mip_maps_pipelines {};
        
        eastl::array<VkSemaphore, glfw_swapchain::NUM_SWAPCHAIN_IMAGES> _deferred_semaphore_image_available = {};
        eastl::array<VkSemaphore, glfw_swapchain::NUM_SWAPCHAIN_IMAGES> _g_buffers_rendering_done = {};
        eastl::array<VkSemaphore, glfw_swapchain::NUM_SWAPCHAIN_IMAGES>  _voxelize_semaphore {};
        eastl::array<VkSemaphore, glfw_swapchain::NUM_SWAPCHAIN_IMAGES> _voxelize_semaphore_done = {};
        

        eastl::array<VkSemaphore, glfw_swapchain::NUM_SWAPCHAIN_IMAGES> _generate_voxel_x_axis_semaphore {};
        eastl::array<VkSemaphore, glfw_swapchain::NUM_SWAPCHAIN_IMAGES> _generate_voxel_y_axis_semaphore = {};
        eastl::array<VkSemaphore, glfw_swapchain::NUM_SWAPCHAIN_IMAGES> _generate_voxel_z_axis_semaphore = {};
        
        eastl::array<VkSemaphore, glfw_swapchain::NUM_SWAPCHAIN_IMAGES> _voxelize_done {};
        
        eastl::array<eastl::array<VkSemaphore, glfw_swapchain::NUM_SWAPCHAIN_IMAGES>, TOTAL_LODS-1> _mip_map_semaphores {};
        
        rendering_mode _rendering_mode = rendering_mode::FULL_RENDERING;
        
        uint32_t _deferred_image_index = 0;
        
        orthographic_camera _ortho_camera;
        screen_plane        _screen_plane;
        
        glm::vec4 _light_color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        
        eastl::array<VkFence, glfw_swapchain::NUM_SWAPCHAIN_IMAGES> _g_buffers_fence {};
        eastl::array<VkFence, glfw_swapchain::NUM_SWAPCHAIN_IMAGES> _voxelize_inflight_fence {};
        eastl::array<VkFence, glfw_swapchain::NUM_SWAPCHAIN_IMAGES> _voxel_command_fence {};
        
        
        static constexpr size_t   NUM_SAMPLING_RAYS = 5;
        
        eastl::array<glm::vec4, NUM_SAMPLING_RAYS> _sampling_rays = {};
        
        static constexpr glm::vec3 _voxel_world_dimensions = glm::vec3(10.0f, 10.0f, 10.0f);
        bool _setup_initialized[glfw_swapchain::NUM_SWAPCHAIN_IMAGES] = {false};
        
    private:
        
        //texture 3d mip maps are not supported moltenvk,so we create our own
        eastl::array< eastl::array<texture_3d, glfw_swapchain::NUM_SWAPCHAIN_IMAGES>,TOTAL_LODS > _voxel_albedo_textures;
        eastl::array< eastl::array<texture_3d, glfw_swapchain::NUM_SWAPCHAIN_IMAGES>,TOTAL_LODS > _voxel_normal_textures;
        
        visual_mat_shared_ptr _debug_material = nullptr;
        
        enum buffer_ids
        {
            NORMALS_ATTACHMENT_ID,
            ALBEDOS_ATTACHMENT_ID,
            POSITIONS_ATTACHMENT_ID,
            PRESENT_ATTACHMENT_ID,
            DEPTH_ATTACHMENT_ID    //DEPTH is always the last attachment and is added automatically
        };
        
        eastl::array<resource_set<render_texture>, 1> _voxel_2d_view {};
        eastl::array<resource_set<render_texture>, 3> _g_buffer_textures {};
        eastl::array<depth_texture, glfw_swapchain::NUM_SWAPCHAIN_IMAGES> _g_buffer_depth {};
        
        VkFence _fence {};
        std::vector<obj_shape*> _shapes;
        
        bool _render_3d_texture = false;
        
    public:
        
        inline void set_render_3d_texture( bool render) { _render_3d_texture = render; }
        glm::vec3 _light_pos = glm::vec3(0.0f, .8f, 0.0f);
        
        inline shader_parameter::shader_params_group& get_mrt_uniform_params(material_base::parameter_stage stage, uint32_t subpass_id, uint32_t binding, int32_t next_frame)
        {
            return _mrt_render_pass.get_subpass(0).get_pipeline( next_frame ).get_uniform_parameters(stage, binding);
        }
        
        inline visual_material::object_shader_params_group& get_mrt_dynamic_params(material_base::parameter_stage stage, uint32_t subpass_id, uint32_t binding, int32_t next_frame)
        {
            return _mrt_render_pass.get_subpass(0).get_pipeline(next_frame).get_dynamic_parameters(stage, binding);
        }
        
        
        inline int get_current_swapchain_image(){ return _deferred_image_index ; }
    };
}
