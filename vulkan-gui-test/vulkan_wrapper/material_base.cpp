//
//  material_base.cpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 7/5/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#include "material_base.h"

using namespace vk;


void material_base::deallocate_parameters()
{
    for (std::pair<parameter_stage , resource::buffer_info > pair : _uniform_buffers)
    {
        vkFreeMemory(_device->_logical_device, pair.second.uniform_buffer_memory, nullptr);
        vkDestroyBuffer(_device->_logical_device, pair.second.uniformBuffer, nullptr);
        
        pair.second.uniformBuffer = VK_NULL_HANDLE;
        pair.second.uniform_buffer_memory = VK_NULL_HANDLE;
    }
}

void material_base::create_descriptor_sets()
{
    VkDescriptorSetAllocateInfo descriptor_set_allocate_info = {};
    
    descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_set_allocate_info.pNext = nullptr;
    descriptor_set_allocate_info.descriptorPool = _descriptor_pool;
    descriptor_set_allocate_info.descriptorSetCount = 1;
    descriptor_set_allocate_info.pSetLayouts = &_descriptor_set_layout;
    
    VkResult result = vkAllocateDescriptorSets(_device->_logical_device, &descriptor_set_allocate_info, &_descriptor_set);
    ASSERT_VULKAN(result);
    std::array<VkWriteDescriptorSet,BINDING_MAX> write_descriptor_sets;
    
    std::array<VkDescriptorBufferInfo, BINDING_MAX> descriptor_buffer_infos;
    std::array<VkDescriptorImageInfo, BINDING_MAX>  descriptor_image_infos;
    
    int count = 0;
    
    for(std::pair<parameter_stage, sampler_parameter > pair : _sampler_parameters)
    {
        for( std::pair<const char*, shader_parameter> pair2 : pair.second)
        {
            descriptor_image_infos[count].sampler = pair2.second.get_image()->get_sampler();
            descriptor_image_infos[count].imageView = pair2.second.get_image()->get_image_view();
            descriptor_image_infos[count].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            
            write_descriptor_sets[count].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write_descriptor_sets[count].pNext = nullptr;
            write_descriptor_sets[count].dstSet = _descriptor_set;
            
            write_descriptor_sets[count].dstBinding = _descriptor_set_layout_bindings[count].binding;
            write_descriptor_sets[count].dstArrayElement = 0;
            write_descriptor_sets[count].descriptorCount = 1;
            write_descriptor_sets[count].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            write_descriptor_sets[count].pImageInfo = &descriptor_image_infos[count];
            write_descriptor_sets[count].pBufferInfo = nullptr;
            write_descriptor_sets[count].pTexelBufferView = nullptr;
            
            ++count;
            
            assert( count < BINDING_MAX);
        }
    }
    
    for (std::pair<parameter_stage , resource::buffer_info > pair : _uniform_buffers)
    {
        assert(usage_type::INVALID != pair.second.usage_type);
        
        descriptor_buffer_infos[count].buffer = pair.second.uniformBuffer;
        descriptor_buffer_infos[count].offset = 0;
        descriptor_buffer_infos[count].range = pair.second.size;
        
        write_descriptor_sets[count].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_descriptor_sets[count].pNext = nullptr;
        write_descriptor_sets[count].dstSet = _descriptor_set;
        
        write_descriptor_sets[count].dstBinding = _descriptor_set_layout_bindings[count].binding;
        write_descriptor_sets[count].dstArrayElement = 0;
        write_descriptor_sets[count].descriptorCount = 1;
        write_descriptor_sets[count].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write_descriptor_sets[count].pImageInfo = nullptr;
        write_descriptor_sets[count].pBufferInfo = &descriptor_buffer_infos[count];
        write_descriptor_sets[count].pTexelBufferView = nullptr;
        
        
        ++count;
        assert( count < BINDING_MAX);
    }
    
    vkUpdateDescriptorSets(_device->_logical_device, count, write_descriptor_sets.data(), 0, nullptr);
}

void material_base::create_descriptor_pool()
{
    std::array<VkDescriptorPoolSize, BINDING_MAX> descriptor_pool_sizes;
    
    int count = 0;
    _samplers_added_on_init = 0;
    for(std::pair<parameter_stage, buffer_parameter > pair : _sampler_buffers)
    {
        for(std::pair<const char*, buffer_info> pair2: pair.second)
        {
            descriptor_pool_sizes[count].type = static_cast<VkDescriptorType>(pair2.second.usage_type);
            descriptor_pool_sizes[count].descriptorCount = 1;
            _samplers_added_on_init++;
            ++count;
            assert(count < BINDING_MAX);
        }
        
    }
    
    for (std::pair<parameter_stage , resource::buffer_info > pair : _uniform_buffers)
    {
        descriptor_pool_sizes[count].type = static_cast<VkDescriptorType>(pair.second.usage_type);
        descriptor_pool_sizes[count].descriptorCount = 1;
        
        ++count;
        assert(count < BINDING_MAX);
    }
    
    assert(count != 0 && "shaders need some sort of input (samplers/uniform buffers)");
    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
    
    descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolCreateInfo.pNext = nullptr;
    descriptorPoolCreateInfo.flags = 0;
    descriptorPoolCreateInfo.maxSets = 1;
    descriptorPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(count);
    descriptorPoolCreateInfo.pPoolSizes = descriptor_pool_sizes.data();
    
    VkResult result = vkCreateDescriptorPool(_device->_logical_device, &descriptorPoolCreateInfo, nullptr, &_descriptor_pool);
    ASSERT_VULKAN(result);
    
}


void material_base::create_descriptor_set_layout()
{
    int count = 0;
    //note: always go through the sampler buffers first, then the uniform buffers because
    //the descriptor bindings will be set up this way.
    for (std::pair<parameter_stage , buffer_parameter > pair : _sampler_buffers)
    {
        for(std::pair<const char*, buffer_info> pair2 : pair.second)
        {
            _descriptor_set_layout_bindings[count].binding = pair2.second.binding;
            _descriptor_set_layout_bindings[count].descriptorType = static_cast<VkDescriptorType>(pair2.second.usage_type);
            _descriptor_set_layout_bindings[count].descriptorCount = 1;
            _descriptor_set_layout_bindings[count].stageFlags = static_cast<VkShaderStageFlagBits>(pair.first);
            _descriptor_set_layout_bindings[count].pImmutableSamplers = nullptr;
            ++count;
            assert(BINDING_MAX > count);
        }
    }
    
    for (std::pair<parameter_stage , resource::buffer_info > pair : _uniform_buffers)
    {
        _descriptor_set_layout_bindings[count].binding = pair.second.binding;
        _descriptor_set_layout_bindings[count].descriptorType = static_cast<VkDescriptorType>(pair.second.usage_type);
        _descriptor_set_layout_bindings[count].descriptorCount = 1;
        _descriptor_set_layout_bindings[count].stageFlags = static_cast<VkShaderStageFlagBits>(pair.first);
        _descriptor_set_layout_bindings[count].pImmutableSamplers = nullptr;
        
        ++count;
        assert(BINDING_MAX > count);
    }
    
    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = {};
    descriptor_set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_set_layout_create_info.pNext = nullptr;
    descriptor_set_layout_create_info.flags = 0;
    
    if(count)
    {
        descriptor_set_layout_create_info.bindingCount = static_cast<uint32_t>(count);
        descriptor_set_layout_create_info.pBindings = _descriptor_set_layout_bindings.data();
        VkResult result = vkCreateDescriptorSetLayout(_device->_logical_device, &descriptor_set_layout_create_info, nullptr, &_descriptor_set_layout);
        
        ASSERT_VULKAN(result);
    }
}

void material_base::init_shader_parameters()
{
    size_t total_size = 0;
    
    _uniform_parameters_added_on_init = 0;
    //note: textures don't need to be initialized here because the texture classes take care of that
    for (std::pair<parameter_stage , buffer_info > pair : _uniform_buffers)
    {
        buffer_info& mem = _uniform_buffers[pair.first];
        shader_parameter::shader_params_group& group = _uniform_parameters[pair.first];
        for (std::pair<const char* , shader_parameter > pair : group)
        {
            //const char* name = pair.first;
            shader_parameter setting = pair.second;
            total_size += setting.get_size_in_bytes();
            ++_uniform_parameters_added_on_init;
        }
        
        _uniform_buffers[pair.first].size = total_size;
        
        if(total_size != 0)
        {
            assert(mem.uniform_buffer_memory == VK_NULL_HANDLE && mem.uniformBuffer == VK_NULL_HANDLE);
            create_buffer(_device->_logical_device, _device->_physical_device, total_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, mem.uniformBuffer,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, mem.uniform_buffer_memory);
        }
        
        
        total_size = 0;
    }
    
    create_descriptor_pool();
    create_descriptor_set_layout();
    create_descriptor_sets();
    
    
    _initialized = true;
}
void material_base::set_image_sampler(image* texture, const char* parameter_name, parameter_stage stage, uint32_t binding, usage_type usage)
{
    buffer_info& mem = _sampler_buffers[stage][parameter_name];
    mem.binding = binding;
    mem.usage_type = usage;

    _sampler_parameters[stage][parameter_name] = texture;
}

void material_base::set_image_smapler(texture_2d* texture, const char* parameter_name, parameter_stage stage, uint32_t binding, usage_type usage)
{
    set_image_sampler(static_cast<image*>(texture), parameter_name, stage, binding, usage);
}

void material_base::set_image_sampler(texture_3d* texture, const char* parameter_name, parameter_stage stage, uint32_t binding, usage_type usage)
{
    set_image_sampler(static_cast<image*>(texture), parameter_name, stage, binding,usage);
}

void material_base::commit_parameters_to_gpu( )
{
    if(!_initialized)
        init_shader_parameters();
    
    uint32_t uniform_parameters_count = 0;
    for (std::pair<parameter_stage , shader_parameter::shader_params_group > pair : _uniform_parameters)
    {
        buffer_info& mem = _uniform_buffers[pair.first];
        shader_parameter::shader_params_group& group = _uniform_parameters[pair.first];
        if(mem.uniform_buffer_memory != VK_NULL_HANDLE)
        {
            if(mem.usage_type == usage_type::UNIFORM_BUFFER)
            {
                void* data = nullptr;
                vkMapMemory(_device->_logical_device, mem.uniform_buffer_memory, 0, mem.size, 0, &data);
                char* byte_data = static_cast<char*>(data);
                
                size_t total_written = 0;
                //important note: this code assumes that in the shader, the parameters are listed in the same order as they
                //appear in the group
                for (std::pair<const char* , shader_parameter > pair : group)
                {
                    assert(pair.second.get_size_in_bytes() != 0 && "Empty parameter, probably means you've left a parameter unassigned");
                    std::memcpy(byte_data + total_written, pair.second.get_stored_value_memory(), pair.second.get_size_in_bytes() );
                    total_written += pair.second.get_size_in_bytes();
                    
                    assert(total_written <= mem.size && "trying to write more memory than that allocated from GPU ");
                    uniform_parameters_count++;
                }
                
                assert(total_written == mem.size && "memory written differs from original memory allocated from GPU, have you added/removed new shader parameters?");
                vkUnmapMemory(_device->_logical_device, mem.uniform_buffer_memory);
            }
        }
        
        assert(uniform_parameters_count == _uniform_parameters_added_on_init && " you've added more uniform parameters after initialization of material, please check code");
        
    }
    
}
