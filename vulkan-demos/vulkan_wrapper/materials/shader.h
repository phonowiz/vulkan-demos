//
//  Shader.hpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 2/12/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#pragma once

#include "resource.h"

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include "device.h"

namespace  vk
{
    class device;
    
    class shader : public resource
    {
    public:
        
        enum class shader_type
        {
            VERTEX = VK_SHADER_STAGE_VERTEX_BIT,
            FRAGMENT = VK_SHADER_STAGE_FRAGMENT_BIT,
            COMPUTE = VK_SHADER_STAGE_COMPUTE_BIT,
            
            //geometry shaders are not supproted on macs as of today
            GEOMETRY = VK_SHADER_STAGE_GEOMETRY_BIT
        };
        
        shader(){};
        shader(device* device, const char* shader_path, shader::shader_type shader_type);
        
        device* _device;
        static const std::string shaderResourcePath;
        

        
        void init( const char *shaderText, shader_type shaderType, const char *entryPoint = "main");
        bool glsl_to_spv(const shader_type shaderType, const char *pshader, std::vector<unsigned int> &spirv);
        
        void init_glsl_lang();
        void finalize_glsl_lang();
        virtual void destroy() override;
        
        inline shader& operator=( const shader& right)
        {
            _device = right._device;
            _pipeline_shader_stage = right._pipeline_shader_stage;
            return *this;
        }
        
        ~shader();
        
        VkPipelineShaderStageCreateInfo _pipeline_shader_stage = {};
    
    };
    
    using shader_shared_ptr = std::shared_ptr<shader>;
};



