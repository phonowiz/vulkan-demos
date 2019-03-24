//
//  Shader.cpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 2/12/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#include "shader.h"
#include "device.h"
#include <vector>


#ifndef __APPLE__
#define VK_USE_PLATFORM_MACOS_MVK
#endif


#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <cstdlib>
#include <iomanip>
#include <fstream>
#include <iostream>
//#include "util.hpp"

#ifdef __ANDROID__
// Android specific include files.
#include <unordered_map>

// Header files.
#include "string.h"
#include "errno.h"
#include <android_native_app_glue.h>
#include "shaderc/shaderc.hpp"
// Static variable that keeps ANativeWindow and asset manager instances.
static android_app *Android_application = nullptr;
#elif (defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK) || defined(__APPLE__))
#include <MoltenVKGLSLToSPIRVConverter/GLSLToSPIRVConverter.h>
#else
#include "SPIRV/GlslangToSpv.h"
#endif

// For timestamp code (get_milliseconds)
#ifdef WIN32
#include <Windows.h>
#else
#include <sys/time.h>
#endif

#include "material.h"
#include <vector>

using namespace vk;

const std::string shader::shaderResourcePath =  "/shaders/";

shader::shader(device* device, const char* filePath, shader::ShaderType shaderType)
{
    std::string  path = resource::resource_root + shader::shaderResourcePath + filePath;
    _device = device;
    
    std::string shader;
    read_file(shader, path);
    
    init(shader.c_str(), shaderType);
}

void shader::init(const char *shaderText, shader::ShaderType shaderType, const char *entryPoint)
{
    VkResult  res;
    bool retVal = false;
    
    assert(shaderText != nullptr);
    
    init_glsl_lang();
    VkShaderModuleCreateInfo moduleCreateInfo;
    
    
    std::vector<unsigned int> vtx_spv;
    _pipelineShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    _pipelineShaderStage.pNext = NULL;
    _pipelineShaderStage.pSpecializationInfo = NULL;
    _pipelineShaderStage.flags = 0;
    _pipelineShaderStage.stage = static_cast<VkShaderStageFlagBits>( shaderType );
    _pipelineShaderStage.pName = entryPoint;
    
    retVal = glsl_to_spv(shaderType, shaderText, vtx_spv);
    assert(retVal);
    
    moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleCreateInfo.pNext = NULL;
    moduleCreateInfo.flags = 0;
    moduleCreateInfo.codeSize = vtx_spv.size() * sizeof(unsigned int);
    moduleCreateInfo.pCode = vtx_spv.data();
    res = vkCreateShaderModule(_device->_logical_device, &moduleCreateInfo, NULL, &_pipelineShaderStage.module);
    assert(res == VK_SUCCESS);
    
    
    finalize_glsl_lang();
}

void shader::init_glsl_lang()
{
#if !defined( __ANDROID__) && !defined(__APPLE__)
    glslang::InitializeProcess();
#endif
}


void shader::finalize_glsl_lang()
{
#if !defined(__ANDROID__) && !defined(__APPLE__)
    glslang::FinalizeProcess();
#endif
}

bool shader::glsl_to_spv(const shader::ShaderType shader_type, const char *pshader, std::vector<unsigned int> &spirv)
{
    MVKShaderStage shaderStage;
    VkShaderStageFlagBits type = static_cast<VkShaderStageFlagBits>(shader_type);
    switch (type) {
        case VK_SHADER_STAGE_VERTEX_BIT:
            shaderStage = kMVKShaderStageVertex;
            break;
        case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
            shaderStage = kMVKShaderStageTessControl;
            break;
        case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
            shaderStage = kMVKShaderStageTessEval;
            break;
        case VK_SHADER_STAGE_GEOMETRY_BIT:
            shaderStage = kMVKShaderStageGeometry;
            break;
        case VK_SHADER_STAGE_FRAGMENT_BIT:
            shaderStage = kMVKShaderStageFragment;
            break;
        case VK_SHADER_STAGE_COMPUTE_BIT:
            shaderStage = kMVKShaderStageCompute;
            break;
        default:
            shaderStage = kMVKShaderStageAuto;
            break;
    }
    
    mvk::GLSLToSPIRVConverter glslConverter;
    glslConverter.setGLSL(pshader);
    bool wasConverted = glslConverter.convert(shaderStage, true, true);
    if (wasConverted) {
        spirv = glslConverter.getSPIRV();
    }
    std::string result = glslConverter.getResultLog();
    if(!result.empty())
    {
        std::cout << "GLSL CONVERSTION FAILED: " <<  std::endl << result << std::endl;
    }
    return wasConverted;
}

void shader::destroy()
{
    vkDestroyShaderModule(_device->_logical_device, _pipelineShaderStage.module, nullptr);
    _pipelineShaderStage.module = VK_NULL_HANDLE;
}

shader::~shader()
{
}


