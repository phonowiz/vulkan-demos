//
//  material_base.cpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 7/5/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#include "material_base.h"
#include <iostream>

using namespace vk;

void material_base::deallocate_parameters()
{
    for (std::pair<parameter_stage , resource::buffer_info > pair : _uniform_buffers)
    {
        vkFreeMemory(_device->_logical_device, pair.second.uniform_buffer_memory, nullptr);
        vkDestroyBuffer(_device->_logical_device, pair.second.uniform_buffer, nullptr);
        
        pair.second.uniform_buffer = VK_NULL_HANDLE;
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
    
    for(std::pair<parameter_stage, sampler_parameter >& pair : _sampler_parameters)
    {
        for( std::pair< std::string_view, shader_parameter> pair2 : pair.second)
        {
            descriptor_image_infos[count].sampler = pair2.second.get_image()->get_sampler();
            descriptor_image_infos[count].imageView = pair2.second.get_image()->get_image_view();
            //TODO: the following needs to change according to the layout the texture is in, it may not always be read_only_optimal
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
        assert(usage_type::UNIFORM_BUFFER == pair.second.usage_type);
        
        descriptor_buffer_infos[count].buffer = pair.second.uniform_buffer;
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
    
    for (std::pair<parameter_stage , material_base::dynamic_buffer_info > pair : _uniform_dynamic_buffers)
    {
        assert(usage_type::INVALID != pair.second.usage_type);
        assert(usage_type::DYNAMIC_UNIFORM_BUFFER == pair.second.usage_type);
        
        descriptor_buffer_infos[count].buffer = pair.second.uniform_buffer;
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
    for(std::pair<parameter_stage, buffer_parameter >& pair : _sampler_buffers)
    {
        for(std::pair<std::string_view, buffer_info> pair2: pair.second)
        {
            descriptor_pool_sizes[count].type = static_cast<VkDescriptorType>(pair2.second.usage_type);
            descriptor_pool_sizes[count].descriptorCount = 1;
            _samplers_added_on_init++;
            ++count;
            assert(count < BINDING_MAX);
        }
    }
    
    for (std::pair<parameter_stage , resource::buffer_info >& pair : _uniform_buffers)
    {
        descriptor_pool_sizes[count].type = static_cast<VkDescriptorType>(pair.second.usage_type);
        descriptor_pool_sizes[count].descriptorCount = 1;
        
        ++count;
        assert(count < BINDING_MAX);
    }
    
    for( std::pair<parameter_stage, dynamic_buffer_info >& pair : _uniform_dynamic_buffers)
    {
        descriptor_pool_sizes[count].type = static_cast<VkDescriptorType>(pair.second.usage_type);
        descriptor_pool_sizes[count].descriptorCount = 1;
        ++count;
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
    for (std::pair<parameter_stage , buffer_parameter > &pair : _sampler_buffers)
    {
        for(std::pair<std::string_view, buffer_info> pair2 : pair.second)
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
    
    for (std::pair<parameter_stage , resource::buffer_info > &pair : _uniform_buffers)
    {
        _descriptor_set_layout_bindings[count].binding = pair.second.binding;
        _descriptor_set_layout_bindings[count].descriptorType = static_cast<VkDescriptorType>(pair.second.usage_type);
        _descriptor_set_layout_bindings[count].descriptorCount = 1;
        _descriptor_set_layout_bindings[count].stageFlags = static_cast<VkShaderStageFlagBits>(pair.first);
        _descriptor_set_layout_bindings[count].pImmutableSamplers = nullptr;
        
        ++count;
        assert(BINDING_MAX > count);
    }
    
    for( std::pair<parameter_stage, dynamic_buffer_info > &pair : _uniform_dynamic_buffers )
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

void material_base::print_uniform_argument_names()
{
    
    std::cout << "printing arguments for material " << this->_name << std::endl;
    for (std::pair<parameter_stage , buffer_info > &pair : _uniform_buffers)
    {

        shader_parameter::shader_params_group& group = _uniform_parameters[pair.first];
        buffer_info& mem = _uniform_buffers[pair.first];
        std::cout << " uniform buffer at binding " << mem.binding << std::endl;
        
        for (std::pair<std::string_view, shader_parameter > pair : group)
        {
            std::string_view name = pair.first;
            std::cout << name <<  std::endl;

        }
    }
}

void material_base::init_shader_parameters()
{
    size_t total_size = 0;
    
    _uniform_parameters_added_on_init = 0;
    //note: textures don't need to be initialized here because the texture classes take care of that
    for (std::pair<parameter_stage , buffer_info > &pair : _uniform_buffers)
    {
        buffer_info& mem = _uniform_buffers[pair.first];
        shader_parameter::shader_params_group& group = _uniform_parameters[pair.first];
        for (std::pair<std::string_view , shader_parameter > pair : group)
        {
            //std::string_view name = pair.first;
            shader_parameter setting = pair.second;
            total_size += setting.get_max_std140_aligned_size_in_bytes();
            ++_uniform_parameters_added_on_init;
        }
        
        _uniform_buffers[pair.first].size = total_size;
        
        if(total_size != 0)
        {
            assert(mem.uniform_buffer_memory == VK_NULL_HANDLE && mem.uniform_buffer == VK_NULL_HANDLE);
            create_buffer(_device->_logical_device, _device->_physical_device, total_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, mem.uniform_buffer,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, mem.uniform_buffer_memory);
        }
        
        total_size = 0;
    }

    //update dynamic parameters
    for (std::pair<parameter_stage , dynamic_buffer_info > pair : _uniform_dynamic_buffers)
    {
        dynamic_buffer_info& mem = _uniform_dynamic_buffers[pair.first];
        object_shader_params_group &obj_group = _uniform_dynamic_parameters[pair.first];
        
        total_size = 0;
        shader_parameter::shader_params_group& group = obj_group[0];
        for (std::pair<std::string_view , shader_parameter > pair : group)
        {
            shader_parameter setting = pair.second;
            total_size += setting.get_max_std140_aligned_size_in_bytes();
        }
        assert(total_size != 0);
        total_size = get_ubo_alignment(total_size);
        total_size *= obj_group.size();
        
        _uniform_dynamic_buffers[pair.first].size = total_size;
        
        if(total_size != 0)
        {
            assert(mem.uniform_buffer_memory == VK_NULL_HANDLE && mem.uniform_buffer == VK_NULL_HANDLE);
            create_buffer(_device->_logical_device, _device->_physical_device, total_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, mem.uniform_buffer,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, mem.uniform_buffer_memory);
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

void material_base::commit_dynamic_parameters_to_gpu()
{
    //todo: we should implement this so that only those objects that have updated get updated, not the whole list of them
    
    assert(_uniform_dynamic_parameters.size() == 0 || _uniform_dynamic_parameters.size() == 1 && "only support 1 dynamic uniform buffer");
    for(std::pair<parameter_stage, object_shader_params_group> pair : _uniform_dynamic_parameters)
    {
        uint32_t uniform_parameters_count = 0;
        uint32_t prev_obj_parameters_count = 0;
        dynamic_buffer_info& mem = _uniform_dynamic_buffers[pair.first];
        
        assert(mem.uniform_buffer_memory != VK_NULL_HANDLE);
        
        void* data = nullptr;
        //todo: avoid mapping every time this function gets called
        vkMapMemory(_device->_logical_device, mem.uniform_buffer_memory, 0, mem.size, 0, &data);
        u_char* start = static_cast<u_char*>(data);
        size_t mem_size = (mem.size);
    
        //for each object id...
        for( std::pair<uint32_t, shader_parameter::shader_params_group> pair2 : pair.second)
        {
            shader_parameter::shader_params_group& group = pair2.second;
            data = static_cast<void*>(start);
            //important note: this code assumes that in the shader, the parameters are listed in the same order as they
            //appear in the group
            for (std::pair<std::string_view , shader_parameter > pair : group)
            {
                data = pair.second.write_to_buffer(data, mem_size);
                uniform_parameters_count++;
                assert(mem_size >= 0);
            }
            
            assert(prev_obj_parameters_count == 0 || prev_obj_parameters_count == uniform_parameters_count && "not all objects have the same amount of dynamic parameters...");
            prev_obj_parameters_count = uniform_parameters_count;
            uniform_parameters_count = 0;
            start += get_dynamic_ubo_stride();
        }
        
        VkMappedMemoryRange mapped_memory_range {};
        mapped_memory_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        mapped_memory_range.memory = mem.uniform_buffer_memory;
        mapped_memory_range.size = mem.size;
        
        vkFlushMappedMemoryRanges(_device->_logical_device, 1, &mapped_memory_range);
        
        vkUnmapMemory(_device->_logical_device, mem.uniform_buffer_memory);
    }
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
                size_t mem_size = (mem.size);
                
                //important note: this code assumes that in the shader, the parameters are listed in the same order as they
                //appear in the group
                for (std::pair<std::string_view , shader_parameter > pair : group)
                {
                    data = pair.second.write_to_buffer(data, mem_size);
                    uniform_parameters_count++;
                    assert(mem_size >= 0);
                }
                vkUnmapMemory(_device->_logical_device, mem.uniform_buffer_memory);
            }
        }
    }
    _uniform_parameters.freeze();
    _sampler_parameters.freeze();
    _uniform_dynamic_parameters.freeze();
    
    commit_dynamic_parameters_to_gpu();

    assert(uniform_parameters_count == _uniform_parameters_added_on_init &&
           " you've added more uniform parameters after initialization of material, please check code");
}

material_base& material_base::operator=( const material_base& right)
{
    if( this != &right)
    {
        _descriptor_set_layout = right._descriptor_set_layout;
        _descriptor_pool = right._descriptor_pool;
        _descriptor_set = right._descriptor_set;
        
        _uniform_buffers = right._uniform_buffers;
        _uniform_parameters = right._uniform_parameters;
        _uniform_dynamic_buffers = right._uniform_dynamic_buffers;
        _uniform_dynamic_parameters = right._uniform_dynamic_parameters;
        _sampler_buffers = right._sampler_buffers;
        _descriptor_set_layout_bindings = right._descriptor_set_layout_bindings;
        _pipeline_shader_stages = right._pipeline_shader_stages;
        _device = right._device;
        _initialized = false;
        _uniform_parameters_added_on_init = right._uniform_parameters_added_on_init;
        _uniform_dynamic_parameters_added_on_init = right._uniform_dynamic_parameters_added_on_init;
        _samplers_added_on_init = right._samplers_added_on_init;
        
        for(int i = 0; i < _pipeline_shader_stages.size(); ++i)
        {
            _pipeline_shader_stages[i] = right._pipeline_shader_stages[i];
        }
    }

    return *this;
}
