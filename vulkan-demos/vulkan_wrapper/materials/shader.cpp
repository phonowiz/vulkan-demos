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

#include "visual_material.h"
#include <vector>

using namespace vk;

const eastl::fixed_string<char, 250> shader::shaderResourcePath =  "/shaders/";

shader::shader(device* device, const char* filePath, shader::shader_type shaderType)
{
    eastl::fixed_string<char, 250>   path = resource::resource_root + shader::shaderResourcePath + filePath;
    _device = device;
    
    std::string shader;
    read_file(shader, path);
    
    init(shader.c_str(), shaderType);
}

void shader::init(const char *shaderText, shader::shader_type shaderType, const char *entryPoint)
{
    VkResult  res;
    bool retVal = false;
    
    assert(shaderText != nullptr);
    
    init_glsl_lang();
    VkShaderModuleCreateInfo module_create_info {};
    
    
    std::vector<unsigned int> vtx_spv;
    _pipeline_shader_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    _pipeline_shader_stage.pNext = NULL;
    _pipeline_shader_stage.pSpecializationInfo = NULL;
    _pipeline_shader_stage.flags = 0;
    _pipeline_shader_stage.stage = static_cast<VkShaderStageFlagBits>( shaderType );
    _pipeline_shader_stage.pName = entryPoint;
    
    retVal = glsl_to_spv(shaderType, shaderText, vtx_spv);
    EA_ASSERT_MSG(retVal, "shader compilation has failed");
    
    module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    module_create_info.pNext = NULL;
    module_create_info.flags = 0;
    module_create_info.codeSize = vtx_spv.size() * sizeof(unsigned int);
    module_create_info.pCode = vtx_spv.data();
    res = vkCreateShaderModule(_device->_logical_device, &module_create_info, NULL, &_pipeline_shader_stage.module);
    EA_ASSERT_MSG(res == VK_SUCCESS, "creation of shader module has failed");
    
    
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

bool shader::glsl_to_spv(const shader::shader_type shader_type, const char *pshader, std::vector<unsigned int> &spirv)
{
    MVKGLSLConversionShaderStage shaderStage;
    VkShaderStageFlagBits type = static_cast<VkShaderStageFlagBits>(shader_type);
    switch (type) {
        case VK_SHADER_STAGE_VERTEX_BIT:
            shaderStage = kMVKGLSLConversionShaderStageVertex;
            break;
        case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
            shaderStage = kMVKGLSLConversionShaderStageTessControl;
            break;
        case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
            shaderStage = kMVKGLSLConversionShaderStageTessEval;
            break;
        case VK_SHADER_STAGE_GEOMETRY_BIT:
            shaderStage = kMVKGLSLConversionShaderStageGeometry;
            break;
        case VK_SHADER_STAGE_FRAGMENT_BIT:
            shaderStage = kMVKGLSLConversionShaderStageFragment;
            break;
        case VK_SHADER_STAGE_COMPUTE_BIT:
            shaderStage = kMVKGLSLConversionShaderStageCompute;
            break;
        default:
            shaderStage = kMVKGLSLConversionShaderStageAuto;
            break;
    }
    
    mvk::GLSLToSPIRVConverter glslConverter;
    glslConverter.setGLSL(pshader);
    bool wasConverted = glslConverter.convert(shaderStage, false, false);
    if (wasConverted) {
        spirv = glslConverter.getSPIRV();
    }
    std::string result = glslConverter.getResultLog();
    if(!wasConverted )
    {
        std::cout << "GLSL CONVERSTION FAILED: " <<  std::endl << result << std::endl;
    }
    return wasConverted;
}

void shader::destroy()
{
    vkDestroyShaderModule(_device->_logical_device, _pipeline_shader_stage.module, nullptr);
    _pipeline_shader_stage.module = VK_NULL_HANDLE;
}

shader::~shader()
{
}


