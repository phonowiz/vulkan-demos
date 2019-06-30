//
//  pipeline.hpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 3/22/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#pragma once

#include "device.h"
#include "material.h"
#include <array>

namespace vk
{
    class texture_2d;
    
    class graphics_pipeline
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
        
        
        graphics_pipeline(device* device, material_shared_ptr material )
        {
            _device = device;
            _material = material;
            
            init_blend_attachments();
        };
        
        graphics_pipeline(device* device)
        {
            _device = device;
            init_blend_attachments();
        }
        
        void create( VkRenderPass render_pass, uint32_t viewport_width, uint32_t viewport_height );
        void set_viewport(uint32_t width, uint32_t height){ _width = width; _height = height;};
        void set_cullmode(cull_mode mode){ _cull_mode = mode; };
        void set_material(material_shared_ptr material ) { _material = material; }
        inline void set_image_sampler(texture_2d* texture, const char* parameter_name, material::parameter_stage parameter_stage, uint32_t binding)
        {
            _material->set_image_sampler(texture, parameter_name, parameter_stage, binding);
        }
        
        inline void set_number_of_blend_attachments(int num_blend_attacments)
        {
            assert( num_blend_attacments != 0 );
            assert( num_blend_attacments <= BLEND_ATTACHMENTS);
            _num_blend_attachments = num_blend_attacments;
        };
        
        void modify_attachment_blend(uint32_t blend_attachment_id, write_channels channels, bool enable_blend )
        {
            assert(blend_attachment_id < _num_blend_attachments);
            _blend_attachments[blend_attachment_id].colorWriteMask = static_cast<VkColorComponentFlags>(channels);
            _blend_attachments[blend_attachment_id].blendEnable = enable_blend ? VK_TRUE : VK_FALSE;
        };
        
        void destroy();
        ~graphics_pipeline(){};
        
    public:
        VkPipeline _pipeline = VK_NULL_HANDLE;
        VkPipelineLayout _pipeline_layout = VK_NULL_HANDLE;
        material_shared_ptr _material = nullptr;
    
    private:
        void init_blend_attachments();
        
    private:
        uint32_t _width = 0;
        uint32_t _height = 0;
        
        cull_mode _cull_mode = cull_mode::BACK_FACE;

        static const uint32_t BLEND_ATTACHMENTS = 10;
        std::array<VkPipelineColorBlendAttachmentState, BLEND_ATTACHMENTS> _blend_attachments {};
        uint32_t _num_blend_attachments = 1;
        
        
        device* _device = nullptr;
        
    };
}
