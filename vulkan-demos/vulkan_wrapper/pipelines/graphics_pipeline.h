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
#include "render_pass.h"
#include "render_texture.h"

namespace vk
{
    class texture_2d;
    
    class graphics_pipeline : public pipeline
    {
    public:
        
        enum class cull_mode
        {
            NONE = VK_CULL_MODE_NONE,
            FRONT_FACE = VK_CULL_MODE_FRONT_BIT,
            BACK_FACE  = VK_CULL_MODE_BACK_BIT,
        };
        
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
        
        enum class front_face
        {
            COUNTER_CLOCKWISE = VK_FRONT_FACE_COUNTER_CLOCKWISE,
            CLOCKWISE = VK_FRONT_FACE_CLOCKWISE
        };
        
        
        graphics_pipeline(){};
        graphics_pipeline(device* device, visual_mat_shared_ptr material ) :
        pipeline(device)
        {
            for(int i = 0; i < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++i)
            {
                _material[i] = material;
            }
            init_blend_attachments();
        };
        
        template< class TEXTURE_TYPE, uint32_t NUM_ATTACHMENTS>
        void create( render_pass<TEXTURE_TYPE, NUM_ATTACHMENTS>& render_pass, uint32_t viewport_width, uint32_t viewport_height );
        
        void set_viewport(uint32_t width, uint32_t height){ _width = width; _height = height;};
        void set_cullmode(cull_mode mode){ _cull_mode = mode; };
        void set_material(visual_mat_shared_ptr material )
        {
            for( int i = 0; i < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++i)
            {
                _material[i] = material;
            }
        }
        
        inline void set_image_sampler(texture_3d& texture, const char* parameter_name,
                                      visual_material::parameter_stage parameter_stage, uint32_t binding, resource::usage_type usage)
        {
            for( int i = 0; i < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++i)
            {
                _material[i]->set_image_sampler(&texture, parameter_name, parameter_stage, binding, usage);
            }
        }
        
        inline void set_image_sampler(texture_2d& texture, const char* parameter_name,
                                      visual_material::parameter_stage parameter_stage, uint32_t binding, resource::usage_type usage)
        {
            for( int i = 0; i < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++i)
            {
                _material[i]->set_image_sampler(&texture, parameter_name, parameter_stage, binding, usage);
            }
        }
        inline void set_image_sampler(std::array<depth_texture, glfw_swapchain::NUM_SWAPCHAIN_IMAGES>& textures, const char* parameter_name,
                                      visual_material::parameter_stage parameter_stage, uint32_t binding, resource::usage_type usage)
        {
            for( int i = 0; i < textures.size(); ++i)
            {
                _material[i]->set_image_sampler(&textures[i], parameter_name, parameter_stage, binding, usage) ;
            }
        }
        
        inline void set_image_sampler(std::array<render_texture, glfw_swapchain::NUM_SWAPCHAIN_IMAGES>& textures, const char* parameter_name,
                                      visual_material::parameter_stage parameter_stage, uint32_t binding, resource::usage_type usage)
        {
            for( int i = 0; i < textures.size(); ++i)
            {
                _material[i]->set_image_sampler(&textures[i], parameter_name, parameter_stage, binding, usage) ;
            }
        }
        
        inline void init_parameter(const char* parameter_name, visual_material::parameter_stage stage, float value, int binding)
        {
            for( int i = 0; i < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++i)
            {
                _material[i]->init_parameter(parameter_name, stage, value, binding);
            }
        };
        
        inline void init_parameter(const char* parameter_name, visual_material::parameter_stage stage, int value, int binding)
        {
            for( int i = 0; i < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++i)
            {
                _material[i]->init_parameter(parameter_name, stage, value, binding);
            }
        };
        inline void init_parameter(const char* parameter_name, visual_material::parameter_stage stage, glm::vec3 value, int binding)
        {
            for( int i = 0; i < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++i)
            {
                _material[i]->init_parameter(parameter_name, stage, value, binding);
            }
        };
        inline void init_parameter(const char* parameter_name, visual_material::parameter_stage stage, glm::vec4 value, int binding)
        {
            for( int i = 0; i < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++i)
            {
                _material[i]->init_parameter(parameter_name, stage, value, binding);
            }
        };
        inline void init_parameter(const char* parameter_name, visual_material::parameter_stage stage, glm::vec2 value, int binding)
        {
            for( int i = 0; i < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++i)
            {
                _material[i]->init_parameter(parameter_name, stage, value, binding);
            }
        }
        inline void init_parameter(const char* parameter_name, visual_material::parameter_stage stage, glm::mat4 value, int binding)
        {
            for( int i = 0; i < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++i)
            {
                _material[i]->init_parameter(parameter_name, stage, value, binding);
            }
        }
        
        inline void init_parameter(const char* parameter_name, visual_material::parameter_stage stage,
                                   glm::vec4* vecs, size_t num_vectors, int binding)
        {
            for( int i = 0; i < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++i)
            {
                _material[i]->init_parameter(parameter_name, stage, vecs, num_vectors, binding);
            }
        }
        
        inline void init_dynamic_params(const char* parameter_name, visual_material::parameter_stage stage, glm::mat4& val, size_t num_objs, int binding)
        {
            
            for( int i = 0; i < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++i)
            {
                for( int j = 0; j < num_objs; ++j)
                    _material[i]->get_dynamic_parameters(stage, binding)[j][parameter_name] = val;
            }
        }
        
        inline void set_image_sampler(std::array<texture_3d, glfw_swapchain::NUM_SWAPCHAIN_IMAGES>& textures, const char* parameter_name,
                                      visual_material::parameter_stage parameter_stage, uint32_t binding, resource::usage_type usage)
        {
            for( int i = 0; i < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++i)
            {
                _material[i]->set_image_sampler(&textures[i], parameter_name, parameter_stage, binding, usage) ;
            }
        }
        
        virtual void commit_parameters_to_gpu(uint32_t swapchain_id) override
        {
            _material[swapchain_id]->commit_parameters_to_gpu();
        }
        
        inline void set_number_of_blend_attachments(uint32_t num_blend_attacments)
        {
            assert( num_blend_attacments <= BLEND_ATTACHMENTS);
            _num_blend_attachments = num_blend_attacments;
        };
        
        
        inline void set_depth_enable(bool enable) { _depth_enable = enable; }
        
        void modify_attachment_blend(uint32_t blend_attachment_id, write_channels channels, bool enable_blend )
        {
            assert(blend_attachment_id < _num_blend_attachments);
            _blend_attachments[blend_attachment_id].colorWriteMask = static_cast<VkColorComponentFlags>(channels);
            _blend_attachments[blend_attachment_id].blendEnable = enable_blend ? VK_TRUE : VK_FALSE;
        }
        
        inline shader_parameter::shader_params_group& get_uniform_parameters(material_base::parameter_stage stage, uint32_t binding, uint32_t swapchain_id)
        {
            return _material[swapchain_id]->get_uniform_parameters(stage, binding);
        }
        
        inline visual_material::object_shader_params_group& get_dynamic_parameters(material_base::parameter_stage stage, uint32_t binding, uint32_t swapchain_id)
        {
            return _material[swapchain_id]->get_dynamic_parameters(stage, binding);
        }
        
        inline void bind_material_assets(uint32_t swapchain_index, VkCommandBuffer& command_buffer, uint32_t object_index)
        {
            uint32_t dynamic_ubo_offset = _material[swapchain_index]->get_dynamic_ubo_stride() * object_index;
            uint32_t offset_count = _material[swapchain_index]->get_dynamic_ubo_stride() == 0 ? 0 : 1;
            vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    _pipeline_layout[swapchain_index], 0, 1, _material[swapchain_index]->get_descriptor_set(), offset_count, &dynamic_ubo_offset);
            
        }
        
        void create_frame_buffer();
        
        ~graphics_pipeline(){};
        
    public:
    
    private:
        void init_blend_attachments();
        
    private:
        
        std::array<visual_mat_shared_ptr, glfw_swapchain::NUM_SWAPCHAIN_IMAGES> _material {};
        uint32_t _width = 0;
        uint32_t _height = 0;
        
        cull_mode _cull_mode = cull_mode::BACK_FACE;
        
        bool _depth_enable = true;
        
        static const uint32_t BLEND_ATTACHMENTS = 10;
        std::array<VkPipelineColorBlendAttachmentState, BLEND_ATTACHMENTS> _blend_attachments {};
        uint32_t _num_blend_attachments = 1;
    };


    template< class TEXTURE_TYPE, uint32_t NUM_ATTACHMENTS>
    void graphics_pipeline::create( render_pass<TEXTURE_TYPE, NUM_ATTACHMENTS>& render_pass, uint32_t viewport_width, uint32_t viewport_height)
    {
        _width = viewport_width;
        _height = viewport_height;

        auto vertex_binding_description = vertex::get_binding_description();
        auto vertex_attribute_descriptos = vertex::get_attribute_descriptions();

        for( int chain_index = 0; chain_index < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++chain_index)
        {
            //note: this call guarantees that material resources are ready to create a pipeline
            _material[chain_index]->commit_parameters_to_gpu();
            
            VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info {};
            vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            vertex_input_state_create_info.pNext = nullptr;
            vertex_input_state_create_info.flags = 0;
            vertex_input_state_create_info.vertexBindingDescriptionCount = 1;
            vertex_input_state_create_info.pVertexBindingDescriptions = &vertex_binding_description;
            vertex_input_state_create_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_attribute_descriptos.size());
            vertex_input_state_create_info.pVertexAttributeDescriptions = vertex_attribute_descriptos.data();


            VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info {};
            input_assembly_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
            input_assembly_create_info.pNext = nullptr;
            input_assembly_create_info.flags = 0;
            input_assembly_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            input_assembly_create_info.primitiveRestartEnable = VK_FALSE;

            VkViewport viewport;
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = _width;
            viewport.height = _height;
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;

            VkRect2D scissor;
            scissor.offset = { 0, 0 };
            scissor.extent = { _width, _height };

            VkPipelineViewportStateCreateInfo viewport_state_create_info {};
            viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            viewport_state_create_info.pNext = nullptr;
            viewport_state_create_info.flags = 0;
            viewport_state_create_info.viewportCount = 1;
            viewport_state_create_info.pViewports = &viewport;
            viewport_state_create_info.scissorCount = 1;
            viewport_state_create_info.pScissors = &scissor;

            VkPipelineRasterizationStateCreateInfo rasterization_state_create_info {};
            rasterization_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            rasterization_state_create_info.pNext = nullptr;
            rasterization_state_create_info.flags = 0;
            rasterization_state_create_info.depthClampEnable = VK_FALSE;
            rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;
            rasterization_state_create_info.polygonMode = VK_POLYGON_MODE_FILL;
            rasterization_state_create_info.cullMode = static_cast<VkCullModeFlagBits>(_cull_mode);

            rasterization_state_create_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
            rasterization_state_create_info.depthBiasEnable = VK_FALSE;
            rasterization_state_create_info.depthBiasConstantFactor = 0.0f;
            rasterization_state_create_info.depthBiasClamp = 0.0f;
            rasterization_state_create_info.depthBiasSlopeFactor = 0.0f;
            rasterization_state_create_info.lineWidth = 1.0f;

            VkPipelineMultisampleStateCreateInfo multisample_state_create_info {};
            multisample_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            multisample_state_create_info.pNext = nullptr;
            multisample_state_create_info.flags = 0;
            multisample_state_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
            multisample_state_create_info.sampleShadingEnable = VK_FALSE;
            multisample_state_create_info.minSampleShading = 1.0f;
            multisample_state_create_info.pSampleMask = nullptr;
            multisample_state_create_info.alphaToCoverageEnable = VK_FALSE;
            multisample_state_create_info.alphaToOneEnable = VK_FALSE;

            VkPipelineDepthStencilStateCreateInfo depth_stencil_create_info = {};

            depth_stencil_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
            depth_stencil_create_info.pNext = nullptr;
            depth_stencil_create_info.flags = 0;
            depth_stencil_create_info.depthTestEnable =  static_cast<VkBool32>(_depth_enable);
            depth_stencil_create_info.depthWriteEnable = static_cast<VkBool32>(_depth_enable);
            depth_stencil_create_info.depthCompareOp = VK_COMPARE_OP_LESS;

            //todo:depth bounds uses the min/max depth bounds below to see if the fragment is within the bounding box
            //we are currently not using this feature
            depth_stencil_create_info.depthBoundsTestEnable = VK_TRUE;
            depth_stencil_create_info.stencilTestEnable = VK_TRUE;
            depth_stencil_create_info.front = {};
            depth_stencil_create_info.back = {};
            depth_stencil_create_info.minDepthBounds = 0.0f;
            depth_stencil_create_info.maxDepthBounds = 1.0f;

            VkPipelineColorBlendStateCreateInfo color_blend_create_info {};
            color_blend_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            color_blend_create_info.pNext = nullptr;
            color_blend_create_info.flags = 0;
            color_blend_create_info.logicOpEnable = VK_FALSE;
            color_blend_create_info.logicOp = VK_LOGIC_OP_NO_OP;
            color_blend_create_info.attachmentCount = _num_blend_attachments;
            color_blend_create_info.pAttachments = _blend_attachments.data();
            color_blend_create_info.blendConstants[0] = 0.0f;
            color_blend_create_info.blendConstants[1] = 0.0f;
            color_blend_create_info.blendConstants[2] = 0.0f;
            color_blend_create_info.blendConstants[3] = 0.0f;


            VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
            pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipeline_layout_create_info.pNext = nullptr;
            pipeline_layout_create_info.flags = 0;
            pipeline_layout_create_info.setLayoutCount = _material[chain_index]->descriptor_set_present() ? 1 : 0;
            pipeline_layout_create_info.pSetLayouts = _material[chain_index]->get_descriptor_set_layout();
            pipeline_layout_create_info.pushConstantRangeCount = 0;
            pipeline_layout_create_info.pPushConstantRanges = nullptr;

            VkResult result = vkCreatePipelineLayout(_device->_logical_device, &pipeline_layout_create_info, nullptr, &_pipeline_layout[chain_index]);
            ASSERT_VULKAN(result);

            VkDynamicState dynamic_state[] = {
                VK_DYNAMIC_STATE_VIEWPORT,
                VK_DYNAMIC_STATE_SCISSOR
            };

            VkPipelineDynamicStateCreateInfo dynamic_state_create_info {};
            dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
            dynamic_state_create_info.pNext = nullptr;
            dynamic_state_create_info.flags = 0;
            dynamic_state_create_info.dynamicStateCount = 2;
            dynamic_state_create_info.pDynamicStates = dynamic_state;

            VkGraphicsPipelineCreateInfo pipeline_create_info {};
            pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            pipeline_create_info.pNext = nullptr;
            pipeline_create_info.flags = 0;
            pipeline_create_info.stageCount = static_cast<uint32_t>(_material[chain_index]->get_shader_stages_size());
            pipeline_create_info.pStages = _material[chain_index]->get_shader_stages();
            pipeline_create_info.pVertexInputState = &vertex_input_state_create_info;
            pipeline_create_info.pInputAssemblyState = &input_assembly_create_info;
            pipeline_create_info.pTessellationState = nullptr;
            pipeline_create_info.pViewportState = &viewport_state_create_info;
            pipeline_create_info.pRasterizationState = &rasterization_state_create_info;
            pipeline_create_info.pMultisampleState = &multisample_state_create_info;
            pipeline_create_info.pDepthStencilState = &depth_stencil_create_info;
            pipeline_create_info.pColorBlendState = &color_blend_create_info;
            pipeline_create_info.pDynamicState = &dynamic_state_create_info;
            pipeline_create_info.layout = _pipeline_layout[chain_index];
            pipeline_create_info.renderPass = render_pass.get_vk_render_pass(chain_index);
            pipeline_create_info.subpass = 0;
            pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
            pipeline_create_info.basePipelineIndex = -1;

            result = vkCreateGraphicsPipelines(_device->_logical_device, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &_pipeline[chain_index]);
        }
    }
}
