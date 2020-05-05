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
    
    template< uint32_t NUM_ATTACHMENTS>
    class graphics_pipeline : public pipeline
    {
    public:

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
            init_blend_attachments();
        };
        
        graphics_pipeline(device* device, glm::vec2 screen_dimensions, visual_mat_shared_ptr material ) :
        pipeline(device)
        {
            _width = screen_dimensions.x;
            _height = screen_dimensions.y;

            _material[0] = material;

            init_blend_attachments();
        };
        

        void create(VkRenderPass& vk_render_pass,uint32_t subpass_id);

        
        void set_viewport(uint32_t width, uint32_t height){ _width = width; _height = height;};
        void set_cull_mode(cull_mode mode){ _cull_mode = mode; };
        void set_material(visual_mat_shared_ptr material )
        {
            _material[0] = material;
        }
        
        inline void add_input_attachment( image* texture, const char* parameter_name,  uint32_t attachment_id,
                                         visual_material::parameter_stage parameter_stage, uint32_t binding)
        {
            _material[0]->add_input_attachment(texture, parameter_name, attachment_id, parameter_stage, binding);
        }
        
        inline void set_image_sampler(texture_3d& texture, const char* parameter_name,
                                      visual_material::parameter_stage parameter_stage, uint32_t binding, resource::usage_type usage)
        {
            _material[0]->set_image_sampler(&texture, parameter_name, parameter_stage, binding, usage);
        }
        
        inline void set_image_sampler(texture_2d& texture, const char* parameter_name,
                                      visual_material::parameter_stage parameter_stage, uint32_t binding, resource::usage_type usage)
        {
            _material[0]->set_image_sampler(&texture, parameter_name, parameter_stage, binding, usage);
        }
        
        inline void init_parameter(const char* parameter_name, visual_material::parameter_stage stage, float value, int binding)
        {
            _material[0]->init_parameter(parameter_name, stage, value, binding);
        };
        
        inline void init_parameter(const char* parameter_name, visual_material::parameter_stage stage, int32_t value, int binding)
        {
            _material[0]->init_parameter(parameter_name, stage, value, binding);
        };
        
        inline void init_parameter(const char* parameter_name, visual_material::parameter_stage stage, uint32_t value, int binding)
        {
            _material[0]->init_parameter(parameter_name, stage, value, binding);
        };
        
        inline void init_parameter(const char* parameter_name, visual_material::parameter_stage stage, glm::vec3 value, int binding)
        {
            _material[0]->init_parameter(parameter_name, stage, value, binding);
        };
        inline void init_parameter(const char* parameter_name, visual_material::parameter_stage stage, glm::vec4 value, int binding)
        {
            _material[0]->init_parameter(parameter_name, stage, value, binding);
            
        };
        inline void init_parameter(const char* parameter_name, visual_material::parameter_stage stage, glm::vec2 value, int binding)
        {
            _material[0]->init_parameter(parameter_name, stage, value, binding);
        }
        inline void init_parameter(const char* parameter_name, visual_material::parameter_stage stage, glm::mat4 value, int binding)
        {
            _material[0]->init_parameter(parameter_name, stage, value, binding);
        }
        
        inline void init_parameter(const char* parameter_name, visual_material::parameter_stage stage,
                                   glm::vec4* vecs, size_t num_vectors, int binding)
        {
            _material[0]->init_parameter(parameter_name, stage, vecs, num_vectors, binding);
        }
        
        inline void init_dynamic_params(const char* parameter_name, visual_material::parameter_stage stage, glm::mat4& val, size_t num_objs, int binding)
        {
            for( int j = 0; j < num_objs; ++j)
                _material[0]->get_dynamic_parameters(stage, binding)[j][parameter_name] = val;
        }
        
        inline void set_image_sampler(std::array<texture_3d, glfw_swapchain::NUM_SWAPCHAIN_IMAGES>& textures, const char* parameter_name,
                                      visual_material::parameter_stage parameter_stage, uint32_t binding, resource::usage_type usage)
        {
            for( int i = 0; i < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++i)
            {
                _material[0]->set_image_sampler(&textures[i], parameter_name, parameter_stage, binding, usage) ;
            }
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
        
        
        void create_frame_buffer();
        
        virtual void destroy() override
        {
            vkDestroyPipeline(_device->_logical_device, _pipeline[0], nullptr);
            vkDestroyPipelineLayout(_device->_logical_device, _pipeline_layout[0], nullptr);
            
            _material[0]->destroy();
            _pipeline[0] = VK_NULL_HANDLE;
            _pipeline_layout[0] = VK_NULL_HANDLE;
        
        }
        
        virtual void commit_parameters_to_gpu()
        {
            _material[0]->commit_parameters_to_gpu();
        }
        
        ~graphics_pipeline(){};
    private:
        void init_blend_attachments();
        
    private:
        
        std::array<visual_mat_shared_ptr, 1> _material {};
        uint32_t _width = 0;
        uint32_t _height = 0;
        
        cull_mode _cull_mode = cull_mode::BACK_FACE;
        
        std::array<VkPipeline, 1 >       _pipeline {};
        std::array<VkPipelineLayout, 1>  _pipeline_layout {};
        bool _depth_enable = true;
        
        static const uint32_t BLEND_ATTACHMENTS = 10;
    
        std::array<VkPipelineColorBlendAttachmentState, BLEND_ATTACHMENTS> _blend_attachments {};
        uint32_t _num_blend_attachments = 1;
    };

    #include "graphics_pipeline.hpp"

    using standard_pipeline = graphics_pipeline<1> ;
}
