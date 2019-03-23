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

namespace vk
{
    class pipeline
    {
    public:
        
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
        
        
        pipeline(device* device, MaterialSharedPtr material ){ _device = device; _material = material; };
        
        void create( VkRenderPass render_pass, uint32_t viewport_width, uint32_t viewport_height );
        
        void set_viewport(uint32_t width, uint32_t height){ _width = width; _height = height;};
        void set_cullmode(cull_mode mode){ _cull_mode = mode; };
        void set_material(MaterialSharedPtr material ) { _material = material; }
        
        //note: there is a ton of stuff that could be exposed through this class, but I will keep it to just what I need as of now
        
        void destroy();
        ~pipeline(){};
        
        
        uint32_t _width = 0;
        uint32_t _height = 0;
        
        cull_mode _cull_mode = cull_mode::BACK_FACE;
        
        VkPipeline _pipeline = VK_NULL_HANDLE;
        VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;
        MaterialSharedPtr _material = nullptr;
        
        device* _device = nullptr;
        
    };
}
