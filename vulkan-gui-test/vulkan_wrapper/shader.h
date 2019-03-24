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
        
        enum class ShaderType
        {
            VERTEX = VK_SHADER_STAGE_VERTEX_BIT,
            FRAGMENT = VK_SHADER_STAGE_FRAGMENT_BIT,
            COMPUTE = VK_SHADER_STAGE_COMPUTE_BIT,
            
            //geometry shaders are not supproted on macs as of today
            GEOMETRY = VK_SHADER_STAGE_GEOMETRY_BIT
        };
        
        shader(device* device, const char* shader_path, shader::ShaderType shader_type);
        
        device* _device;
        static const std::string shaderResourcePath;
        

        
        void initShader( const char *shaderText, ShaderType shaderType, const char *entryPoint = "main");
        bool GLSLtoSPV(const ShaderType shaderType, const char *pshader, std::vector<unsigned int> &spirv);
        
        void initGLSLang();
        void finalizeGLSLang();
        virtual void destroy() override;
        ~shader();
        
        VkPipelineShaderStageCreateInfo _pipelineShaderStage = {};
    
    };
    
    using ShaderSharedPtr = std::shared_ptr<shader>;
};



