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

Material::Material( const char* name, ShaderSharedPtr vertexShader, ShaderSharedPtr fragmentShader, PhysicalDevice* device)
{
    _vertexShader = vertexShader;
    _fragmentShader = fragmentShader;
    _name = name;
    _device = device;
}

void Material::commitParametersToGPU( )
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
                vkMapMemory(_device->_device, mem.uniformBufferMemory, 0, mem.size, 0, &data);
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
                vkUnmapMemory(_device->_device, mem.uniformBufferMemory);
            }
            if(mem.usageType == UsageType::COMBINED_IMAGE_SAMPLER)
            {
                
                //todo: textures need to be implemented here
                assert(0);
            }
        }

    }

}

ShaderParameter::ShaderParamsGroup& Material::getImageSamplerParameters(ParameterStage stage)
{
    
    std::vector<BufferInfo> &mems = _samplerBuffers[stage];
    
    assert(0); //no implemnetation as of right now
}
ShaderParameter::ShaderParamsGroup& Material::getUniformParameters(ParameterStage stage)
{
    BufferInfo& mem = _uniformBuffers[stage];
    
    mem.usageType = UsageType::UNIFORM_BUFFER;
    return _uniformParameters[stage];
    
}

void Material::deallocateParameters()
{
    for (std::pair<ParameterStage , Resource::BufferInfo > pair : _uniformBuffers)
    {
        vkFreeMemory(_device->_device, pair.second.uniformBufferMemory, nullptr);
        vkDestroyBuffer(_device->_device, pair.second.uniformBuffer, nullptr);
        
        pair.second.uniformBuffer = VK_NULL_HANDLE;
        pair.second.uniformBufferMemory = VK_NULL_HANDLE;
    }
}
//todo: this should be a private function?
void Material::initShaderParameters()
{
    size_t totalSize = 0;
    
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
            createBuffer(_device->_device, _device->_physicalDevice, totalSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, mem.uniformBuffer,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, mem.uniformBufferMemory);
        }

        
        totalSize = 0;
    }
    createDescriptorPool();
    createDescriptorSetLayout();
    createDescriptorSet();
 
    
    initialized = true;
}

void Material::createDescriptorSet()
{
    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
    
    descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocateInfo.pNext = nullptr;
    descriptorSetAllocateInfo.descriptorPool = _descriptorPool;
    descriptorSetAllocateInfo.descriptorSetCount = 1;
    descriptorSetAllocateInfo.pSetLayouts = &_descriptorSetLayout;
    
    VkResult result = vkAllocateDescriptorSets(_device->_device, &descriptorSetAllocateInfo, &_descriptorSet);
    ASSERT_VULKAN(result);
    std::array<VkWriteDescriptorSet,BINDING_MAX> writeDescriptorSets;

    std::array<VkDescriptorBufferInfo, BINDING_MAX> descriptorbufferInfos;
    std::array<VkDescriptorImageInfo, BINDING_MAX>  descriptorImageInfos;
    
    int count = 0;
    
    for (std::pair<ParameterStage , Resource::BufferInfo > pair : _uniformBuffers)
    {
        assert(UsageType::INVALID != pair.second.usageType);
        if(pair.second.usageType == UsageType::UNIFORM_BUFFER)
        {
            descriptorbufferInfos[count].buffer = pair.second.uniformBuffer;
            descriptorbufferInfos[count].offset = 0;
            descriptorbufferInfos[count].range = pair.second.size;
            
            writeDescriptorSets[count].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeDescriptorSets[count].pNext = nullptr;
            writeDescriptorSets[count].dstSet = _descriptorSet;

            writeDescriptorSets[count].dstBinding = _descriptorSets[count].binding;
            writeDescriptorSets[count].dstArrayElement = 0;
            writeDescriptorSets[count].descriptorCount = 1;
            writeDescriptorSets[count].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            writeDescriptorSets[count].pImageInfo = nullptr;
            writeDescriptorSets[count].pBufferInfo = &descriptorbufferInfos[count];
            writeDescriptorSets[count].pTexelBufferView = nullptr;
        }
        else if(pair.second.usageType == UsageType::COMBINED_IMAGE_SAMPLER)
        {

            /*
            descriptorImageInfos[count].sampler = shroomImage.getSampler();
            descriptorImageInfos[count].imageView = shroomImage.getImageView();
            descriptorImageInfos[count].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
             */
            assert(0); //todo: you need to implement this
            
            writeDescriptorSets[count].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeDescriptorSets[count].pNext = nullptr;
            writeDescriptorSets[count].dstSet = _descriptorSet;
            
            writeDescriptorSets[count].dstBinding = _descriptorSets[count].binding;
            writeDescriptorSets[count].dstArrayElement = 0;
            writeDescriptorSets[count].descriptorCount = 1;
            writeDescriptorSets[count].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writeDescriptorSets[count].pImageInfo = &descriptorImageInfos[count];
            writeDescriptorSets[count].pBufferInfo = nullptr;//&descriptorbufferInfos[count];
            writeDescriptorSets[count].pTexelBufferView = nullptr;
        }

        ++count;
    }
    
    vkUpdateDescriptorSets(_device->_device, count, writeDescriptorSets.data(), 0, nullptr);
}

void Material::createDescriptorSetLayout()
{
    
    VkDescriptorSetLayoutBinding descriptorSetLayoutBinding = {};
   
    int count = 0;
    for (std::pair<ParameterStage , Resource::BufferInfo > pair : _uniformBuffers)
    {

        assert(BINDING_MAX > count);
        
        //this is the uniform buffer containing model/view/projection information
        descriptorSetLayoutBinding.binding = static_cast<uint32_t>(count);
        descriptorSetLayoutBinding.descriptorType = static_cast<VkDescriptorType>(pair.second.usageType);
        descriptorSetLayoutBinding.descriptorCount = 1;
        descriptorSetLayoutBinding.stageFlags = static_cast<VkShaderStageFlagBits>(pair.first);
        descriptorSetLayoutBinding.pImmutableSamplers = nullptr;
        
        _descriptorSets[count] = descriptorSetLayoutBinding;
        
        ++count;
    }
    
    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
    descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutCreateInfo.pNext = nullptr;
    descriptorSetLayoutCreateInfo.flags = 0;
    
    if(count)
    {
        descriptorSetLayoutCreateInfo.bindingCount = static_cast<uint32_t>(count);
        descriptorSetLayoutCreateInfo.pBindings = _descriptorSets.data();
        VkResult result = vkCreateDescriptorSetLayout(_device->_device, &descriptorSetLayoutCreateInfo, nullptr, &_descriptorSetLayout);
        
        ASSERT_VULKAN(result);
    }

}

void Material::createDescriptorPool()
{
    std::array<VkDescriptorPoolSize, BINDING_MAX> descriptorPoolSizes;
    
    int count = 0;
    for (std::pair<ParameterStage , Resource::BufferInfo > pair : _uniformBuffers)
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
        
        VkResult result = vkCreateDescriptorPool(_device->_device, &descriptorPoolCreateInfo, nullptr, &_descriptorPool);
        ASSERT_VULKAN(result);
    }
}

void Material::destroy()
{
    vkDestroyDescriptorSetLayout(_device->_device, _descriptorSetLayout, nullptr);
    vkDestroyDescriptorPool(_device->_device, _descriptorPool, nullptr);
    _descriptorSetLayout = VK_NULL_HANDLE;
    _descriptorPool = VK_NULL_HANDLE;
    
    deallocateParameters();
}
Material::~Material()
{
}
