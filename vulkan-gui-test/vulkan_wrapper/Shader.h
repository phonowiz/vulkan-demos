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
#include "physical_device.h"

namespace  vk
{
    class PhysicalDevice;
    
    class Shader : public Resource
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
        
        Shader(PhysicalDevice* physicalDevice, const char* shaderPath, Shader::ShaderType shaderType);
        
        PhysicalDevice* _physicalDevice;
        static const std::string shaderResourcePath;
        

        
        void initShader( const char *shaderText, ShaderType shaderType, const char *entryPoint = "main");
        bool GLSLtoSPV(const ShaderType shaderType, const char *pshader, std::vector<unsigned int> &spirv);
        
        void initGLSLang();
        void finalizeGLSLang();
        virtual void destroy() override;
        ~Shader();
        
        VkPipelineShaderStageCreateInfo _pipelineShaderStage = {};
    
    };
    
    using ShaderSharedPtr = std::shared_ptr<Shader>;
};



