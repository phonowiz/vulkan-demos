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
    _vertex_shader = vertexShader;
    _fragment_shader = fragmentShader;
    _name = name;
    _device = device;
}

void material::commit_parameters_to_gpu( )
{
//    BufferInfo mem =  _uniformBuffers[parameterStage];
//    ShaderParameter::ShaderParamsGroup& group = _parameters[parameterStage];
//
    if(!initialized)
        init_shader_parameters();
    
    for (std::pair<parameter_stage , shader_parameter::shader_params_group > pair : _uniform_parameters)
    {
        buffer_info& mem = _uniform_buffers[pair.first];
        shader_parameter::shader_params_group& group = _uniform_parameters[pair.first];
        if(mem.uniformBufferMemory != VK_NULL_HANDLE)
        {
            if(mem.usageType == usage_type::UNIFORM_BUFFER)
            {
                void* data = nullptr;
                vkMapMemory(_device->_logical_device, mem.uniformBufferMemory, 0, mem.size, 0, &data);
                char* byteData = static_cast<char*>(data);
                
                size_t totalWritten = 0;
                //important note: this code assumes that in the shader, the parameters are listed in the same order as they
                //appear in the group
                for (std::pair<const char* , shader_parameter > pair : group)
                {
                    assert(pair.second.get_size_in_bytes() != 0 && "Empty parameter, probably means you've left a parameter unassigned");
                    memcpy(byteData + totalWritten, pair.second.get_stored_value_memory(), pair.second.get_size_in_bytes() );
                    totalWritten += pair.second.get_size_in_bytes();
                    
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

void material::set_image_sampler(texture_2d* texture, const char* parameterName, parameter_stage parameterStage, uint32_t binding)
{
    buffer_info& mem = _sampler_buffers[parameterStage];
    mem.binding = binding;
    mem.usageType = usage_type::COMBINED_IMAGE_SAMPLER;
    
    _sampler_parameters[parameterStage][parameterName] = texture;
}
shader_parameter::shader_params_group& material::get_uniform_parameters(parameter_stage stage, uint32_t binding)
{
    buffer_info& mem = _uniform_buffers[stage];
    mem.binding = binding;
    mem.usageType = usage_type::UNIFORM_BUFFER;
    
    return _uniform_parameters[stage];
    
}

void material::deallocate_parameters()
{
    for (std::pair<parameter_stage , resource::buffer_info > pair : _uniform_buffers)
    {
        vkFreeMemory(_device->_logical_device, pair.second.uniformBufferMemory, nullptr);
        vkDestroyBuffer(_device->_logical_device, pair.second.uniformBuffer, nullptr);
        
        pair.second.uniformBuffer = VK_NULL_HANDLE;
        pair.second.uniformBufferMemory = VK_NULL_HANDLE;
    }
}
//todo: there needs to be away for client to init parameters
void material::init_shader_parameters()
{
    size_t totalSize = 0;
    
    //note: textures don't need to be initialized here because the texture classes take care of that
    for (std::pair<parameter_stage , buffer_info > pair : _uniform_buffers)
    {
        buffer_info& mem = _uniform_buffers[pair.first];
        shader_parameter::shader_params_group& group = _uniform_parameters[pair.first];
        for (std::pair<const char* , shader_parameter > pair : group)
        {
            //const char* name = pair.first;
            shader_parameter setting = pair.second;
            totalSize += setting.get_size_in_bytes();
        }
        
        _uniform_buffers[pair.first].size = totalSize;
        
        if(totalSize != 0)
        {
            assert(mem.uniformBufferMemory == VK_NULL_HANDLE && mem.uniformBuffer == VK_NULL_HANDLE);
            create_buffer(_device->_logical_device, _device->_physical_device, totalSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, mem.uniformBuffer,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, mem.uniformBufferMemory);
        }

        
        totalSize = 0;
    }
    
    create_descriptor_pool();
    create_descriptor_set_layout();
    create_descriptor_set();
 
    
    initialized = true;
}

void material::create_descriptor_set()
{
    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
    
    descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocateInfo.pNext = nullptr;
    descriptorSetAllocateInfo.descriptorPool = _descriptor_pool;
    descriptorSetAllocateInfo.descriptorSetCount = 1;
    descriptorSetAllocateInfo.pSetLayouts = &_descriptor_set_layout;
    
    VkResult result = vkAllocateDescriptorSets(_device->_logical_device, &descriptorSetAllocateInfo, &_descriptor_set);
    ASSERT_VULKAN(result);
    std::array<VkWriteDescriptorSet,BINDING_MAX> writeDescriptorSets;

    std::array<VkDescriptorBufferInfo, BINDING_MAX> descriptorbufferInfos;
    std::array<VkDescriptorImageInfo, BINDING_MAX>  descriptorImageInfos;
    
    int count = 0;
    
    for(std::pair<parameter_stage, sampler_parameter > pair : _sampler_parameters)
    {
        for( std::pair<const char*, shader_parameter> pair2 : pair.second)
        {
            //TODO: what about sampler 3D?
            texture_2d* texture = pair2.second.get_texture_2d();
            descriptorImageInfos[count].sampler = texture->get_sampler();
            descriptorImageInfos[count].imageView = texture->get_image_view();
            descriptorImageInfos[count].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            writeDescriptorSets[count].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeDescriptorSets[count].pNext = nullptr;
            writeDescriptorSets[count].dstSet = _descriptor_set;

            writeDescriptorSets[count].dstBinding = _descriptor_set_layout_bindings[count].binding;
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

    for (std::pair<parameter_stage , resource::buffer_info > pair : _uniform_buffers)
    {
        assert(usage_type::INVALID != pair.second.usageType);

        descriptorbufferInfos[count].buffer = pair.second.uniformBuffer;
        descriptorbufferInfos[count].offset = 0;
        descriptorbufferInfos[count].range = pair.second.size;
        
        writeDescriptorSets[count].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[count].pNext = nullptr;
        writeDescriptorSets[count].dstSet = _descriptor_set;

        writeDescriptorSets[count].dstBinding = _descriptor_set_layout_bindings[count].binding;
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

void material::create_descriptor_set_layout()
{
    int count = 0;
    //note: always go through the sampler buffers first, then the uniform buffers because
    //the descriptor bindings will be set up this way.
    for (std::pair<parameter_stage , resource::buffer_info > pair : _sampler_buffers)
    {
        _descriptor_set_layout_bindings[count].binding = pair.second.binding;
        _descriptor_set_layout_bindings[count].descriptorType = static_cast<VkDescriptorType>(pair.second.usageType);
        _descriptor_set_layout_bindings[count].descriptorCount = 1;
        _descriptor_set_layout_bindings[count].stageFlags = static_cast<VkShaderStageFlagBits>(pair.first);
        _descriptor_set_layout_bindings[count].pImmutableSamplers = nullptr;
        
        ++count;
        assert(BINDING_MAX > count);
    }
    
    for (std::pair<parameter_stage , resource::buffer_info > pair : _uniform_buffers)
    {
        _descriptor_set_layout_bindings[count].binding = pair.second.binding;
        _descriptor_set_layout_bindings[count].descriptorType = static_cast<VkDescriptorType>(pair.second.usageType);
        _descriptor_set_layout_bindings[count].descriptorCount = 1;
        _descriptor_set_layout_bindings[count].stageFlags = static_cast<VkShaderStageFlagBits>(pair.first);
        _descriptor_set_layout_bindings[count].pImmutableSamplers = nullptr;
        
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
        descriptorSetLayoutCreateInfo.pBindings = _descriptor_set_layout_bindings.data();
        VkResult result = vkCreateDescriptorSetLayout(_device->_logical_device, &descriptorSetLayoutCreateInfo, nullptr, &_descriptor_set_layout);
        
        ASSERT_VULKAN(result);
    }

}

void material::create_descriptor_pool()
{
    std::array<VkDescriptorPoolSize, BINDING_MAX> descriptorPoolSizes;
    
    int count = 0;
    
    for(std::pair<parameter_stage, resource::buffer_info > pair : _sampler_buffers)
    {
        descriptorPoolSizes[count].type = static_cast<VkDescriptorType>(pair.second.usageType);
        descriptorPoolSizes[count].descriptorCount = 1;
        
        ++count;
        assert(count < BINDING_MAX);
    }
    
    for (std::pair<parameter_stage , resource::buffer_info > pair : _uniform_buffers)
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
        
        VkResult result = vkCreateDescriptorPool(_device->_logical_device, &descriptorPoolCreateInfo, nullptr, &_descriptor_pool);
        ASSERT_VULKAN(result);
    }
}

void material::destroy()
{
    vkDestroyDescriptorSetLayout(_device->_logical_device, _descriptor_set_layout, nullptr);
    vkDestroyDescriptorPool(_device->_logical_device, _descriptor_pool, nullptr);
    _descriptor_set_layout = VK_NULL_HANDLE;
    _descriptor_pool = VK_NULL_HANDLE;
    
    deallocate_parameters();
}
material::~material()
{
}
