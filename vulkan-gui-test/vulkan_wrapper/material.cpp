//
//  material.cpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 2/10/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#include "material.h"
#include "shader_parameter.h"
#include <unordered_map>
#include <array>
#include <fstream>
#include <iostream>

using namespace vk;

material::material( const char* name, ShaderSharedPtr vertexShader, ShaderSharedPtr fragmentShader, device* device)
{
    _vertexShader = vertexShader;
    _fragmentShader = fragmentShader;
    _name = name;
    _device = device;
}

void material::commitParametersToGPU( )
{
//    BufferInfo mem =  _uniformBuffers[parameterStage];
//    ShaderParameter::ShaderParamsGroup& group = _parameters[parameterStage];
//
    if(!initialized)
        initShaderParameters();
    
    for (std::pair<ParameterStage , ShaderParameter::ShaderParamsGroup > pair : _uniformParameters)
    {
        BufferInfo& mem = _uniformBuffers[pair.first];
        ShaderParameter::ShaderParamsGroup& group = _uniformParameters[pair.first];
        if(mem.uniformBufferMemory != VK_NULL_HANDLE)
        {
            if(mem.usageType == UsageType::UNIFORM_BUFFER)
            {
                void* data = nullptr;
                vkMapMemory(_device->_logical_device, mem.uniformBufferMemory, 0, mem.size, 0, &data);
                char* byteData = static_cast<char*>(data);
                
                size_t totalWritten = 0;
                //important note: this code assumes that in the shader, the parameters are listed in the same order as they
                //appear in the group
                for (std::pair<const char* , ShaderParameter > pair : group)
                {
                    assert(pair.second.getByteSize() != 0 && "Empty parameter, probably means you've left a parameter unassigned");
                    memcpy(byteData + totalWritten, pair.second.getStoredValueMemory(), pair.second.getByteSize() );
                    totalWritten += pair.second.getByteSize();
                    
                    assert(totalWritten <= mem.size && "trying to write more memory than that allocated from GPU ");
                }
                
                assert(totalWritten == mem.size && "memory written differs from original memory allocated from GPU, have you added/removed new shader parameters?");
                vkUnmapMemory(_device->_logical_device, mem.uniformBufferMemory);
            }
        }

    }

}

//ShaderParameter::ShaderParamsGroup& Material::getImageSamplerParameters(ParameterStage stage, uint32_t binding)
//{
//
//    BufferInfo &mems = _samplerBuffers[stage];
//    mems.usageType = UsageType::COMBINED_IMAGE_SAMPLER;
//    mems.binding = binding;
//
//    return _samplerParameters[stage];
//}

void material::setImageSampler(texture_2d* texture, const char* parameterName, ParameterStage parameterStage, uint32_t binding)
{
    BufferInfo& mem = _samplerBuffers[parameterStage];
    mem.binding = binding;
    mem.usageType = UsageType::COMBINED_IMAGE_SAMPLER;
    
    _samplerParameters[parameterStage][parameterName] = texture;
}
ShaderParameter::ShaderParamsGroup& material::getUniformParameters(ParameterStage stage, uint32_t binding)
{
    BufferInfo& mem = _uniformBuffers[stage];
    mem.binding = binding;
    mem.usageType = UsageType::UNIFORM_BUFFER;
    
    return _uniformParameters[stage];
    
}

void material::deallocateParameters()
{
    for (std::pair<ParameterStage , resource::BufferInfo > pair : _uniformBuffers)
    {
        vkFreeMemory(_device->_logical_device, pair.second.uniformBufferMemory, nullptr);
        vkDestroyBuffer(_device->_logical_device, pair.second.uniformBuffer, nullptr);
        
        pair.second.uniformBuffer = VK_NULL_HANDLE;
        pair.second.uniformBufferMemory = VK_NULL_HANDLE;
    }
}
//todo: there needs to be away for client to init parameters
void material::initShaderParameters()
{
    size_t totalSize = 0;
    
    //note: textures don't need to be initialized here because the texture classes take care of that
    for (std::pair<ParameterStage , BufferInfo > pair : _uniformBuffers)
    {
        BufferInfo& mem = _uniformBuffers[pair.first];
        ShaderParameter::ShaderParamsGroup& group = _uniformParameters[pair.first];
        for (std::pair<const char* , ShaderParameter > pair : group)
        {
            //const char* name = pair.first;
            ShaderParameter setting = pair.second;
            totalSize += setting.getByteSize();
        }
        
        _uniformBuffers[pair.first].size = totalSize;
        
        if(totalSize != 0)
        {
            assert(mem.uniformBufferMemory == VK_NULL_HANDLE && mem.uniformBuffer == VK_NULL_HANDLE);
            createBuffer(_device->_logical_device, _device->_physical_device, totalSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, mem.uniformBuffer,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, mem.uniformBufferMemory);
        }

        
        totalSize = 0;
    }
    
    createDescriptorPool();
    createDescriptorSetLayout();
    createDescriptorSet();
 
    
    initialized = true;
}

void material::createDescriptorSet()
{
    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
    
    descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocateInfo.pNext = nullptr;
    descriptorSetAllocateInfo.descriptorPool = _descriptorPool;
    descriptorSetAllocateInfo.descriptorSetCount = 1;
    descriptorSetAllocateInfo.pSetLayouts = &_descriptorSetLayout;
    
    VkResult result = vkAllocateDescriptorSets(_device->_logical_device, &descriptorSetAllocateInfo, &_descriptorSet);
    ASSERT_VULKAN(result);
    std::array<VkWriteDescriptorSet,BINDING_MAX> writeDescriptorSets;

    std::array<VkDescriptorBufferInfo, BINDING_MAX> descriptorbufferInfos;
    std::array<VkDescriptorImageInfo, BINDING_MAX>  descriptorImageInfos;
    
    int count = 0;
    
    for(std::pair<ParameterStage, SamplerParameter > pair : _samplerParameters)
    {
        for( std::pair<const char*, ShaderParameter> pair2 : pair.second)
        {
            //TODO: what about sampler 3D?
            texture_2d* texture = pair2.second.getSampler2DValue();
            descriptorImageInfos[count].sampler = texture->getSampler();
            descriptorImageInfos[count].imageView = texture->getImageView();
            descriptorImageInfos[count].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            writeDescriptorSets[count].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeDescriptorSets[count].pNext = nullptr;
            writeDescriptorSets[count].dstSet = _descriptorSet;

            writeDescriptorSets[count].dstBinding = _descriptorSetLayoutBindings[count].binding;
            writeDescriptorSets[count].dstArrayElement = 0;
            writeDescriptorSets[count].descriptorCount = 1;
            writeDescriptorSets[count].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writeDescriptorSets[count].pImageInfo = &descriptorImageInfos[count];
            writeDescriptorSets[count].pBufferInfo = nullptr;
            writeDescriptorSets[count].pTexelBufferView = nullptr;

            ++count;

            assert( count < BINDING_MAX);
        }
    }

    for (std::pair<ParameterStage , resource::BufferInfo > pair : _uniformBuffers)
    {
        assert(UsageType::INVALID != pair.second.usageType);

        descriptorbufferInfos[count].buffer = pair.second.uniformBuffer;
        descriptorbufferInfos[count].offset = 0;
        descriptorbufferInfos[count].range = pair.second.size;
        
        writeDescriptorSets[count].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[count].pNext = nullptr;
        writeDescriptorSets[count].dstSet = _descriptorSet;

        writeDescriptorSets[count].dstBinding = _descriptorSetLayoutBindings[count].binding;
        writeDescriptorSets[count].dstArrayElement = 0;
        writeDescriptorSets[count].descriptorCount = 1;
        writeDescriptorSets[count].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writeDescriptorSets[count].pImageInfo = nullptr;
        writeDescriptorSets[count].pBufferInfo = &descriptorbufferInfos[count];
        writeDescriptorSets[count].pTexelBufferView = nullptr;


        ++count;
        assert( count < BINDING_MAX);
    }
    
    vkUpdateDescriptorSets(_device->_logical_device, count, writeDescriptorSets.data(), 0, nullptr);
}

void material::createDescriptorSetLayout()
{
    int count = 0;
    //note: always go through the sampler buffers first, then the uniform buffers because
    //the descriptor bindings will be set up this way.
    for (std::pair<ParameterStage , resource::BufferInfo > pair : _samplerBuffers)
    {
        _descriptorSetLayoutBindings[count].binding = pair.second.binding;
        _descriptorSetLayoutBindings[count].descriptorType = static_cast<VkDescriptorType>(pair.second.usageType);
        _descriptorSetLayoutBindings[count].descriptorCount = 1;
        _descriptorSetLayoutBindings[count].stageFlags = static_cast<VkShaderStageFlagBits>(pair.first);
        _descriptorSetLayoutBindings[count].pImmutableSamplers = nullptr;
        
        ++count;
        assert(BINDING_MAX > count);
    }
    
    for (std::pair<ParameterStage , resource::BufferInfo > pair : _uniformBuffers)
    {
        _descriptorSetLayoutBindings[count].binding = pair.second.binding;
        _descriptorSetLayoutBindings[count].descriptorType = static_cast<VkDescriptorType>(pair.second.usageType);
        _descriptorSetLayoutBindings[count].descriptorCount = 1;
        _descriptorSetLayoutBindings[count].stageFlags = static_cast<VkShaderStageFlagBits>(pair.first);
        _descriptorSetLayoutBindings[count].pImmutableSamplers = nullptr;
        
        ++count;
        assert(BINDING_MAX > count);
    }

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
    descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutCreateInfo.pNext = nullptr;
    descriptorSetLayoutCreateInfo.flags = 0;
    
    if(count)
    {
        descriptorSetLayoutCreateInfo.bindingCount = static_cast<uint32_t>(count);
        descriptorSetLayoutCreateInfo.pBindings = _descriptorSetLayoutBindings.data();
        VkResult result = vkCreateDescriptorSetLayout(_device->_logical_device, &descriptorSetLayoutCreateInfo, nullptr, &_descriptorSetLayout);
        
        ASSERT_VULKAN(result);
    }

}

void material::createDescriptorPool()
{
    std::array<VkDescriptorPoolSize, BINDING_MAX> descriptorPoolSizes;
    
    int count = 0;
    
    for(std::pair<ParameterStage, resource::BufferInfo > pair : _samplerBuffers)
    {
        descriptorPoolSizes[count].type = static_cast<VkDescriptorType>(pair.second.usageType);
        descriptorPoolSizes[count].descriptorCount = 1;
        
        ++count;
        assert(count < BINDING_MAX);
    }
    
    for (std::pair<ParameterStage , resource::BufferInfo > pair : _uniformBuffers)
    {
        descriptorPoolSizes[count].type = static_cast<VkDescriptorType>(pair.second.usageType);
        descriptorPoolSizes[count].descriptorCount = 1;
        
        ++count;
        assert(count < BINDING_MAX);
    }
    

    
    if(count)
    {
        VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
        
        descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptorPoolCreateInfo.pNext = nullptr;
        descriptorPoolCreateInfo.flags = 0;
        descriptorPoolCreateInfo.maxSets = 1;
        descriptorPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(count);
        descriptorPoolCreateInfo.pPoolSizes = descriptorPoolSizes.data();
        
        VkResult result = vkCreateDescriptorPool(_device->_logical_device, &descriptorPoolCreateInfo, nullptr, &_descriptorPool);
        ASSERT_VULKAN(result);
    }
}

void material::destroy()
{
    vkDestroyDescriptorSetLayout(_device->_logical_device, _descriptorSetLayout, nullptr);
    vkDestroyDescriptorPool(_device->_logical_device, _descriptorPool, nullptr);
    _descriptorSetLayout = VK_NULL_HANDLE;
    _descriptorPool = VK_NULL_HANDLE;
    
    deallocateParameters();
}
material::~material()
{
}
