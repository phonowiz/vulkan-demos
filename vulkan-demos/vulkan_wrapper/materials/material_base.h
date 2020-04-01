//
//  material_base.hpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 7/5/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#pragma once

#include "resource.h"
#include "shader_parameter.h"
#include "ordered_map.h"
#include "depth_texture.h"
#include <array>
#include <vector>
#include <assert.h>

namespace vk
{
    /*
     ***** Vulkan review ******
     
     Descriptors are the atomic unit that represent one bind in a shader.  Descriptors aren't bound individually, but rather in
     blocks of memory called descriptor sets.
     
     A descriptor set layout specifies information of the bindings that are found in the descriptor set.  You create a descriptor set layout by using
     a descriptor set layout binding structure which specifies how the bindings will be used and in which stage the bound descriptor will be used.  However,
     these descriptor set layouts do not tell vulkan what actual texture or buffer will be used in the shader.
     
     To specify which resource (texture, buffers, etc.  These are called opaque resources in the vulkan literature) will be bound so that the shader can do it's work, you have to write
     the resource memory by using the function vkUpdateDescriptorSets.   Please check the function material::create_descriptor_sets for an
     example of how this works.
     
     ****** About vk::material_base ***
     
     This class acts as a wrapper around all things materials, including shaders, descriptors, descriptor sets, and descriptor sets
     bindings.
     
     Each material class has one descriptor set, and this set used to describe to vulkan all the resources the vertex and fragment
     shaders passed in upon creation of vk::material_base classes need in order to work properly.
     
     */
    
    class material_base : public resource
    {
    public:
        
        material_base()
        {}
        material_base( device* device, const char* name ){ _device = device; _name = name; }
        
        enum class parameter_stage
        {
            VERTEX =    VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT,
            FRAGMENT =  VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT,
            COMPUTE =   VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT,
            NONE =      VkShaderStageFlagBits::VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM
        };
        
    public:
        void commit_parameters_to_gpu();
        void commit_dynamic_parameters_to_gpu();
        void print_uniform_argument_names();
        
    protected:
        void init_shader_parameters();
        void create_descriptor_set_layout();
        void create_descriptor_pool();
        void create_descriptor_sets();
        void deallocate_parameters();
        
        inline size_t get_ubo_alignment( size_t mem_size )
        {
            return (mem_size + _device->get_properties().limits.minUniformBufferOffsetAlignment - 1 ) &
            ~(_device->get_properties().limits.minUniformBufferOffsetAlignment - 1);
        }
        
    public:
        
        inline bool descriptor_set_present() { return _descriptor_set_layout != VK_NULL_HANDLE; }
        inline VkDescriptorSetLayout* get_descriptor_set_layout(){ return &_descriptor_set_layout; }
        inline VkDescriptorSet* get_descriptor_set(){ return &_descriptor_set; }
        
        
        inline size_t get_num_dynamic_buffer_elements(){ return _uniform_dynamic_buffers.size(); }
        
        inline uint32_t get_dynamic_ubo_stride()
        {
            uint32_t bytes = 0;
            //todo: we only support one uniform dynamic buffer per material, but I think that's all we need....
            assert(_uniform_dynamic_parameters.size() == 0 || _uniform_dynamic_parameters.size() == 1);
            for(std::pair<parameter_stage, object_shader_params_group> pair : _uniform_dynamic_parameters)
            {
                dynamic_buffer_info& mem = _uniform_dynamic_buffers[pair.first];
                bytes = static_cast<uint32_t>(mem.size / pair.second.size() );
            }
            
            return bytes;
        };
        
        virtual material_base& operator=( const material_base& right);
        void set_in_use(){ _in_use = true; }
        bool get_in_use(){ return _in_use; }
        
        virtual void destroy() override;
        void set_image_sampler(image* texture, const char* parameter_name, parameter_stage stage, uint32_t binding, usage_type usage);
        void set_image_smapler(texture_2d* texture, const char* parameter_name, parameter_stage stage, uint32_t binding, usage_type usage);
        void set_image_sampler(texture_3d* texture, const char* parameter_name, parameter_stage stage, uint32_t binding, usage_type usage);
        void set_vec4_array(glm::vec4* vec4s, size_t, const char* parameter_name, parameter_stage stage, uint32_t binding, usage_type usage);
        
        virtual VkPipelineShaderStageCreateInfo* get_shader_stages() = 0;
        virtual size_t get_shader_stages_size() = 0;
        virtual const char* const *get_instance_type() = 0;
        const char* _name = nullptr;
    
    public:
            using object_shader_params_group = ordered_map<uint32_t, shader_parameter::shader_params_group >  ;
    protected:
        
        struct dynamic_buffer_info : public resource::buffer_info
        {
            shader_parameter::Type type = shader_parameter::Type::NONE;
            size_t parameters_size = 0;
            dynamic_buffer_info()
            {}
        };
        
        VkDescriptorSetLayout _descriptor_set_layout =  VK_NULL_HANDLE;
        VkDescriptorPool      _descriptor_pool =        VK_NULL_HANDLE;
        VkDescriptorSet       _descriptor_set =         VK_NULL_HANDLE;
        
        //TODO: check out the VkPhysicalDeviceLimits structure: https://vulkan.lunarg.com/doc/view/1.0.30.0/linux/vkspec.chunked/ch31s02.html
        static const int BINDING_MAX = 30;
        
        ordered_map<parameter_stage, resource::buffer_info>                       _uniform_buffers;
        ordered_map<parameter_stage, shader_parameter::shader_params_group>       _uniform_parameters;
        

        
        //todo: for dynamic buffers, I think we won't need parameter stage, verify that that statement is true
        //dynamic uniform buffers are shared by the stages
        ordered_map<parameter_stage, material_base::dynamic_buffer_info >          _uniform_dynamic_buffers;
        ordered_map<parameter_stage, object_shader_params_group >                  _uniform_dynamic_parameters;

        
        typedef ordered_map<const char*, resource::buffer_info>             buffer_parameter;
        ordered_map<parameter_stage, buffer_parameter>                      _sampler_buffers;

        
        typedef ordered_map< const char*, shader_parameter>                      sampler_parameter;
        ordered_map<parameter_stage, sampler_parameter>                          _sampler_parameters;
        std::array<VkDescriptorSetLayoutBinding, BINDING_MAX>                    _descriptor_set_layout_bindings;
        
        static const size_t MAX_SHADER_STAGES = 2;
        std::array<VkPipelineShaderStageCreateInfo, MAX_SHADER_STAGES>           _pipeline_shader_stages;
        
        bool _initialized = false;
        device* _device = nullptr;
        
        uint32_t _uniform_parameters_added_on_init = 0;
        uint32_t _uniform_dynamic_parameters_added_on_init = 0;
        uint32_t _samplers_added_on_init = 0;
        bool _in_use = false;
    };
    
    using mat_shared_ptr = std::shared_ptr<material_base>;
}
