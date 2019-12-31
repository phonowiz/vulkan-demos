//
//  render_pass.hpp
//  vulkan-demos
//
//  Created by Rafael Sabino on 12/28/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#pragma once

#include <vector>
#include <array>

#define VK_USE_PLATFORM_MACOS_MVK
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_CTOR_INIT
#define GLM_FORCE_SILENT_WARNINGS

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include "../core/swapchain.h"
#include "../core/object.h"

namespace vk
{
    class texture_2d;
    class depth_texture;
    class obj_shape;
    class device;

    class render_pass : public object
    {
    public:
        render_pass(device* device, bool _off_screen_rendering, glm::vec2 dimensions);
        
        void set_rendering_attachments(std::array<std::vector<texture_2d*>, swapchain::NUM_SWAPCHAIN_IMAGES>& rendering_texture);
        inline void set_clear_value(size_t attachment_index, glm::vec4 value)
        {
            assert(_vk_frame_buffer_infos.size() > attachment_index);
            _vk_frame_buffer_infos[attachment_index]._clear_value.color.float32[0] = value.x;
            _vk_frame_buffer_infos[attachment_index]._clear_value.color.float32[1] = value.y;
            _vk_frame_buffer_infos[attachment_index]._clear_value.color.float32[2] = value.z;
            _vk_frame_buffer_infos[attachment_index]._clear_value.color.float32[3] = value.w;
        }
        
        
        inline void set_depth_clear_value(size_t attachment_index, float depth_clear, uint32_t stencil_clear )
        {
            assert(_vk_frame_buffer_infos.size() > attachment_index);
            _vk_frame_buffer_infos[attachment_index]._clear_value.depthStencil.depth= depth_clear;
            _vk_frame_buffer_infos[attachment_index]._clear_value.depthStencil.stencil =  stencil_clear;
        }
        
        void set_depth_attachments(std::array<depth_texture*, vk::swapchain::NUM_SWAPCHAIN_IMAGES>& depths){_depth_textures = &depths;};
        
        void init();
        
        void record_draw_commands(obj_shape**, size_t num_shapes);
        
        inline VkRenderPass get_render_pass(size_t i)
        {
            assert( i < _vk_render_passes.size());
            return _vk_render_passes[i];
            
        };
        
        void destroy() override;
    private:
        
        void create_frame_buffers();
        
    private:
        
        struct frame_buffer_info
        {
            VkFramebuffer _frame_buffer = VK_NULL_HANDLE;
            VkClearValue  _clear_value = {};
        };
        
        //note: we rely on NUM_SWAPCHAIN_IMAGES to declare in these arrays, in the future if that is no longer
        //the way to go, templatize this class.
        
        std::array<std::vector<texture_2d*>, vk::swapchain::NUM_SWAPCHAIN_IMAGES>*  _attachments = nullptr;
        std::array<depth_texture*, vk::swapchain::NUM_SWAPCHAIN_IMAGES>*             _depth_textures = nullptr;
        std::array<VkRenderPass, swapchain::NUM_SWAPCHAIN_IMAGES>       _vk_render_passes {};
        std::array<frame_buffer_info, swapchain::NUM_SWAPCHAIN_IMAGES>  _vk_frame_buffer_infos {};
        
        bool _off_screen_rendering = true;

        static constexpr uint32_t MAX_NUMBER_OF_ATTACHMENTS = 5;
        

        glm::vec2 _dimensions {};
        device* _device = nullptr;
        
    };
}
