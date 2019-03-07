//
//  material.hpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 2/10/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#pragma once

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include <array>

#include "physical_device.h"
#include "shader.h"
#include "shader_parameter.h"
#include "tsl/ordered_map.h"
#include <array>


namespace  vk
{
    class Material : public Resource
    {
    public:
        Material(const char* name, ShaderSharedPtr vertexShader, ShaderSharedPtr fragmentShader, PhysicalDevice* device );
        
        //currently we oly support 3 shader stages max: vertex, pixel, and compute. geometry is not supported on macs.
        //compute pipeline is not yet implemented in this code.
        std::array<VkPipelineShaderStageCreateInfo,2> getShaderStages()
        {
            std::array<VkPipelineShaderStageCreateInfo, 2> stages;
            stages[0] = _vertexShader->_pipelineShaderStage;
            stages[1] = _fragmentShader->_pipelineShaderStage;
            
            return stages;
        }
        
        enum class ParameterStage
        {
            VERTEX = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT,
            FRAGMENT = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT,
            COMPUTE = VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT
        };
        
//        int SetParameteri(const char* parameterName, int const value);
//        int SetParameterui(const char* parameterName, unsigned int const value);
//        int SetParameterv4(const char* parameterName, const glm::vec4 &value);
//        int SetParameterv3(const char* parameterName, const glm::vec3 & value);
//        int SetParameterv2(const char* parameterName, const glm::vec2 & value);
//        int SetParameterf(const char* parameterName, float const value);
//        int SetParamatermat4(const char *parameterName, const glm::mat4 &value);
//        int SetParameterSampler2D(const GLchar* parameterName, Texture2D* sampler);
//        int SetParameterSampler3D(const GLchar* parameterName, Texture3D* sampler);
//        int SetPointLight(const char* parameterName,  const PointLight& light  );
//        int SetMatrix(const char* parameterName, const glm::mat4& mat);
//        int SetParameterBool(const char* parameterName, bool value);
        
//        int ActivateTexture2D(const GLchar* samplerName, const int textureName, const int textureUnit);
//        int ActivateTexture3D(const GLchar* samplerName, const int textureName, const int textureUnit);
//        int ActivateTexture2D(const GLchar* samplerName, const Texture2D* texture, const int textureUnit);
//        int ActivateTexture3D(const GLchar* samplerName, const Texture3D* texture, const int textureUnit);
        
        
        void initShaderParameters();
        void commitParametersToGPU();
        
        //todo: maybe this should be called setupParameterBinding, because the layout describes the type of data that is
        //bound to the shader
        void createDescriptorSetLayout();
        void createDescriptorPool();
        void createDescriptorSet();
        void deallocateParameters();
        virtual void destroy() override;
        
        inline VkDescriptorSetLayout* getDesciptorSetLayout(){ return &_descriptorSetLayout; }
        inline VkDescriptorSet* getDescriptorSet(){ return &_descriptorSet; }
        
        ShaderParameter::ShaderParamsGroup& getUniformParameters(ParameterStage parameterStage);
        ShaderParameter::ShaderParamsGroup& getImageSamplerParameters(ParameterStage parameterStage);
        
        ShaderSharedPtr _vertexShader;
        ShaderSharedPtr _fragmentShader;
        
        
        VkDescriptorSetLayout _descriptorSetLayout = VK_NULL_HANDLE;
        VkDescriptorPool      _descriptorPool = VK_NULL_HANDLE;
        VkDescriptorSet       _descriptorSet = VK_NULL_HANDLE;
        
        static const int BINDING_MAX = 30;
        
        
        tsl::ordered_map<ParameterStage, Resource::BufferInfo>                  _uniformBuffers;
        tsl::ordered_map<ParameterStage, ShaderParameter::ShaderParamsGroup>    _uniformParameters;

        tsl::ordered_map<ParameterStage, std::vector<Resource::BufferInfo>>                  _samplerBuffers;
        tsl::ordered_map<ParameterStage, std::vector<ShaderParameter::ShaderParamsGroup>>    _samplerParameters;
        std::array<VkDescriptorSetLayoutBinding, BINDING_MAX>                   _descriptorSets;
        
        
        const char* _name = nullptr;
        
        PhysicalDevice *_device = nullptr;
        
        
        bool initialized = false;
        ~Material();
        

        
    private:
        
//        VkBuffer       _uniformBuffer = VK_NULL_HANDLE;
//        VkDeviceMemory _uniformBufferMemory = VK_NULL_HANDLE;
    };
    
    
    using MaterialSharedPtr = std::shared_ptr<Material>;
}
