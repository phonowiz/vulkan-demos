//
//  pipeline.hpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 3/22/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#pragma once

#include "device.h"
#include "visual_material.h"
#include "pipeline.h"
#include "resource.h"
#include <array>
//#include "render_pass.h"
#include "render_texture.h"

//#include "graphics_pipeline.h"
#include "vertex.h"

namespace vk
{
    class texture_2d;
    
    template<class RENDER_TEXTURE_TYPE, uint32_t NUM_ATTACHMENTS>
    class graphics_pipeline : public pipeline
    {
    public:
        
        //using render_pass_type = render_pass<RENDER_TEXTURE_TYPE,NUM_ATTACHMENTS>;
        //using subpass_type = typename render_pass_type::subpass_s;

        enum class write_channels
        {
            R = VK_COLOR_COMPONENT_R_BIT,
            G = VK_COLOR_COMPONENT_G_BIT,
            B = VK_COLOR_COMPONENT_B_BIT,
            A = VK_COLOR_COMPONENT_A_BIT,
            RG = R | G,
            RGB = RG | B,
            RGBA = RGB | A
        };
        
        enum class cull_mode
        {
            NONE = VK_CULL_MODE_NONE,
            FRONT_FACE = VK_CULL_MODE_FRONT_BIT,
            BACK_FACE  = VK_CULL_MODE_BACK_BIT,
        };
        

        enum class front_face
        {
            COUNTER_CLOCKWISE = VK_FRONT_FACE_COUNTER_CLOCKWISE,
            CLOCKWISE = VK_FRONT_FACE_CLOCKWISE
        };
        
        
        graphics_pipeline()
        {
            //set_clear_attachments_colors(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
            //set_clear_depth(glm::vec2(1.0f, 0.0f));
            init_blend_attachments();
        };
        
        graphics_pipeline(device* device, glm::vec2 screen_dimensions, visual_mat_shared_ptr material ) :
        pipeline(device)
        //_render_pass(device, screen_dimensions)
        {
            _width = screen_dimensions.x;
            _height = screen_dimensions.y;
            
            //for(int i = 0; i < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++i)
            {
                _material[0] = material;
            }
            
//            set_clear_attachments_colors(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
//            set_clear_depth(glm::vec2(1.0f, 0.0f));
            init_blend_attachments();
        };
        

        void create(VkRenderPass& vk_render_pass);

//        void set_clear_depth( glm::vec2 depth_stencil)
//        {
//            //note: depth value is always the last one...
//            _clear_values[_clear_values.size()-1].depthStencil = {depth_stencil.x, (uint32_t)depth_stencil.y};
//
//        }
//        void set_clear_attachments_colors( glm::vec4 color)
//        {
//            assert(_depth_enable && "you must enable depth for this to take effect");
//            //note" -2 because the last one is depth attachment
//            for(int i = 0; i < _clear_values.size()-2; ++i)
//            {
//                _clear_values[i].color = {color.x, color.y, color.z, color.w};
//            }
//        }
        
        void set_viewport(uint32_t width, uint32_t height){ _width = width; _height = height;};
        void set_cull_mode(cull_mode mode){ _cull_mode = mode; };
        void set_material(visual_mat_shared_ptr material )
        {
            //for( int i = 0; i < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++i)
            {
                _material[0] = material;
            }
        }
        
//        void set_clear_attachments_colors( glm::vec4 color)
//        {
//            assert(_depth_enable && "you must enable depth for this to take effect");
//            //note" -2 because the last one is depth attachment
//            for(int i = 0; i < _clear_values.size()-2; ++i)
//            {
//                _clear_values[i].color = {color.x, color.y, color.z, color.w};
//            }
//        }
        
//        inline void set_depth_enable(bool enable)
//        {
//            _render_pass.set_depth_enable(enable);
//        }
        
//        inline void set_rendering_attachments(std::array<std::array<RENDER_TEXTURE_TYPE, glfw_swapchain::NUM_SWAPCHAIN_IMAGES>,
//                                                                                   NUM_ATTACHMENTS>& rendering_textures)
//        {
//            _render_pass.set_rendering_attachments(rendering_textures);
//        }
//        subpass_type& add_subpass(){ return _render_pass.add_subpass(); }
        
//        inline VkFramebuffer get_vk_frame_buffer(uint32_t i)
//        {
//            return _render_pass.get_vk_frame_buffer(i);
//        }
        
//        inline VkRenderPass get_vk_render_pass(uint32_t i)
//        {
//            return _render_pass.get_vk_render_pass(i);
//        }
//        inline std::array<depth_texture, glfw_swapchain::NUM_SWAPCHAIN_IMAGES>& get_depth_textures(){ return _render_pass.get_depth_textures(); }
        
//        inline void set_offscreen_rendering( bool offscreen_rendering)
//        {
//            _render_pass.set_offscreen_rendering(offscreen_rendering);
//        }
        inline void set_image_sampler(texture_3d& texture, const char* parameter_name,
                                      visual_material::parameter_stage parameter_stage, uint32_t binding, resource::usage_type usage)
        {
            //for( int i = 0; i < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++i)
            {
                _material[0]->set_image_sampler(&texture, parameter_name, parameter_stage, binding, usage);
            }
        }
        
        inline void set_image_sampler(texture_2d& texture, const char* parameter_name,
                                      visual_material::parameter_stage parameter_stage, uint32_t binding, resource::usage_type usage)
        {
            //for( int i = 0; i < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++i)
            {
                _material[0]->set_image_sampler(&texture, parameter_name, parameter_stage, binding, usage);
            }
        }
        inline void set_image_sampler(std::array<depth_texture, glfw_swapchain::NUM_SWAPCHAIN_IMAGES>& textures, const char* parameter_name,
                                      visual_material::parameter_stage parameter_stage, uint32_t binding, resource::usage_type usage)
        {
            for( int i = 0; i < textures.size(); ++i)
            {
                _material[0]->set_image_sampler(&textures[i], parameter_name, parameter_stage, binding, usage) ;
            }
        }
        
        inline void set_image_sampler(std::array<render_texture, glfw_swapchain::NUM_SWAPCHAIN_IMAGES>& textures, const char* parameter_name,
                                      visual_material::parameter_stage parameter_stage, uint32_t binding, resource::usage_type usage)
        {
            for( int i = 0; i < textures.size(); ++i)
            {
                _material[0]->set_image_sampler(&textures[i], parameter_name, parameter_stage, binding, usage) ;
            }
        }
        
        inline void init_parameter(const char* parameter_name, visual_material::parameter_stage stage, float value, int binding)
        {
            //for( int i = 0; i < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++i)
            {
                _material[0]->init_parameter(parameter_name, stage, value, binding);
            }
        };
        
        inline void init_parameter(const char* parameter_name, visual_material::parameter_stage stage, int value, int binding)
        {
            //for( int i = 0; i < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++i)
            {
                _material[0]->init_parameter(parameter_name, stage, value, binding);
            }
        };
        inline void init_parameter(const char* parameter_name, visual_material::parameter_stage stage, glm::vec3 value, int binding)
        {
            //for( int i = 0; i < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++i)
            {
                _material[0]->init_parameter(parameter_name, stage, value, binding);
            }
        };
        inline void init_parameter(const char* parameter_name, visual_material::parameter_stage stage, glm::vec4 value, int binding)
        {
            //for( int i = 0; i < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++i)
            {
                _material[0]->init_parameter(parameter_name, stage, value, binding);
            }
        };
        inline void init_parameter(const char* parameter_name, visual_material::parameter_stage stage, glm::vec2 value, int binding)
        {
            //for( int i = 0; i < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++i)
            {
                _material[0]->init_parameter(parameter_name, stage, value, binding);
            }
        }
        inline void init_parameter(const char* parameter_name, visual_material::parameter_stage stage, glm::mat4 value, int binding)
        {
            //for( int i = 0; i < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++i)
            {
                _material[0]->init_parameter(parameter_name, stage, value, binding);
            }
        }
        
        inline void init_parameter(const char* parameter_name, visual_material::parameter_stage stage,
                                   glm::vec4* vecs, size_t num_vectors, int binding)
        {
            //for( int i = 0; i < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++i)
            {
                _material[0]->init_parameter(parameter_name, stage, vecs, num_vectors, binding);
            }
        }
        
        inline void init_dynamic_params(const char* parameter_name, visual_material::parameter_stage stage, glm::mat4& val, size_t num_objs, int binding)
        {
            
            //for( int i = 0; i < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++i)
            {
                for( int j = 0; j < num_objs; ++j)
                    _material[0]->get_dynamic_parameters(stage, binding)[j][parameter_name] = val;
            }
        }
        
        inline void set_image_sampler(std::array<texture_3d, glfw_swapchain::NUM_SWAPCHAIN_IMAGES>& textures, const char* parameter_name,
                                      visual_material::parameter_stage parameter_stage, uint32_t binding, resource::usage_type usage)
        {
            for( int i = 0; i < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++i)
            {
                _material[0]->set_image_sampler(&textures[i], parameter_name, parameter_stage, binding, usage) ;
            }
        }
        
        void commit_parameters_to_gpu()
        {
            commit_parameters_to_gpu(0);
        }
        
        inline void set_number_of_blend_attachments(uint32_t num_blend_attacments)
        {
            assert( num_blend_attacments <= BLEND_ATTACHMENTS);
            _num_blend_attachments = num_blend_attacments;
        };
        
        void modify_attachment_blend(uint32_t blend_attachment_id, write_channels channels, bool enable_blend )
        {
            assert(blend_attachment_id < _num_blend_attachments);
            _blend_attachments[blend_attachment_id].colorWriteMask = static_cast<VkColorComponentFlags>(channels);
            _blend_attachments[blend_attachment_id].blendEnable = enable_blend ? VK_TRUE : VK_FALSE;
        }
        
        inline shader_parameter::shader_params_group& get_uniform_parameters(material_base::parameter_stage stage, uint32_t binding)
        {
            return _material[0]->get_uniform_parameters(stage, binding);
        }
        
        inline visual_material::object_shader_params_group& get_dynamic_parameters(material_base::parameter_stage stage, uint32_t binding)
        {
            return _material[0]->get_dynamic_parameters(stage, binding);
        }
        
        inline VkPipeline& get_vk_pipeline(){ return _pipeline[0]; }
        inline void bind_material_assets(VkCommandBuffer& command_buffer, uint32_t object_index)
        {
            uint32_t dynamic_ubo_offset = _material[0]->get_dynamic_ubo_stride() * object_index;
            uint32_t offset_count = _material[0]->get_dynamic_ubo_stride() == 0 ? 0 : 1;
            vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    _pipeline_layout[0], 0, 1, _material[0]->get_descriptor_set(), offset_count, &dynamic_ubo_offset);
            
        }
        
//        bool is_initialized()
//        {
//            bool result = true;
//            for( int i =0; i < _pipeline.size(); ++i)
//            {
//                if(_pipeline[i] == VK_NULL_HANDLE || _pipeline_layout[i] == VK_NULL_HANDLE)
//                {
//                    result = false;
//                    break;
//                }
//            }
//            return result;
//        }
        
        void create_frame_buffer();
//        virtual void destroy() override
//        {
//            vk::pipeline::destroy();
//
////            //_render_pass.destroy();
//        }
        
        //void begin_command_recording(VkCommandBuffer& buffer, VkRenderPass render_pass, VkFramebuffer frame_buffer);
        //void end_command_recording();
        
        //typename render_pass_type::subpass_s&  create_subpass(){ return _render_pass.add_subpass(); }
        
        virtual void destroy() override
        {
            //for(int i = 0; i < _pipeline.size(); ++i)
            {
                vkDestroyPipeline(_device->_logical_device, _pipeline[0], nullptr);
                vkDestroyPipelineLayout(_device->_logical_device, _pipeline_layout[0], nullptr);
                
                _pipeline[0] = VK_NULL_HANDLE;
                _pipeline_layout[0] = VK_NULL_HANDLE;
            }
        }
        
        ~graphics_pipeline(){};
    private:
        void init_blend_attachments();
        
        virtual void commit_parameters_to_gpu(uint32_t) override
        {
            _material[0]->commit_parameters_to_gpu();
        }
        
    private:
        
        std::array<visual_mat_shared_ptr, /*glfw_swapchain::NUM_SWAPCHAIN_IMAGES*/ 1> _material {};
        uint32_t _width = 0;
        uint32_t _height = 0;
        
        cull_mode _cull_mode = cull_mode::BACK_FACE;
        
//        VkCommandBuffer* _recording_buffer = VK_NULL_HANDLE;
        
        std::array<VkPipeline, 1 >       _pipeline {};
        std::array<VkPipelineLayout, 1>  _pipeline_layout {};
        bool _depth_enable = true;
        
        static const uint32_t BLEND_ATTACHMENTS = 10;
    
        //std::array<VkClearValue,NUM_ATTACHMENTS + 1> _clear_values {};
        std::array<VkPipelineColorBlendAttachmentState, BLEND_ATTACHMENTS> _blend_attachments {};
        uint32_t _num_blend_attachments = 1;
        
        //render_pass_type _render_pass;
    };

    #include "graphics_pipeline.hpp"

    using standard_pipeline = graphics_pipeline<glfw_present_texture, 1> ;
}
