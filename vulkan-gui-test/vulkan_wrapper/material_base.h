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
#include "tsl/ordered_map.h"

#include <array>

namespace vk
{
    /*
     ***** Vulkan review ******
     
     Descriptors are the atomic unit that represent one bind in a shader.  Descriptors aren't bound individually, but rather in
     blocks of memory called descriptor sets.
     
     A descriptor set layout specifies information of the bindings that are found in the descriptor set.
     An example of this information would be how the binding will be used and in which stage the bound descriptor will be used.  However,
     these descriptor set layouts do not tell vulkan what actual texture or buffer will be used in the shader.
     
     To specify which resource (texture, buffers, etc) will be bound so that the shader can do it's work, you have to "write"
     the resource memory by using the function vkUpdateDescriptorSets.   Please check the function material::create_descriptor_sets for an
     example of how this works.
     
     ****** About vk::material ***
     
     This class acts as a wrapper around all things materials, including shaders, descriptors, descriptor sets, and descriptor sets
     bindings.
     
     Each material class has one descriptor set, and this set used to describe to vulkan all the resources the vertex and fragment
     shaders passed in upon creation of vk::material need in order to work properly.
     
     */
    
    class material_base : public resource
    {
    public:
        
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
        
    protected:
        void init_shader_parameters();

        
        void create_descriptor_set_layout();
        void create_descriptor_pool();
        void create_descriptor_sets();
        void deallocate_parameters();
        
    public:
        
        inline bool descriptor_set_present() { return _descriptor_set_layout != VK_NULL_HANDLE; }
        inline VkDescriptorSetLayout* get_descriptor_set_layout(){ return &_descriptor_set_layout; }
        inline VkDescriptorSet* get_descriptor_set(){ return &_descriptor_set; }
        
        virtual void destroy() override {   _initialized = false; }
        void set_image_sampler(image* texture, const char* parameter_name, parameter_stage stage, uint32_t binding);
        void set_image_smapler(texture_2d* texture, const char* parameter_name, parameter_stage stage, uint32_t binding);
        void set_image_sampler(texture_3d* texture, const char* parameter_name, parameter_stage stage, uint32_t binding);
        
        virtual VkPipelineShaderStageCreateInfo* get_shader_stages() = 0;
        virtual size_t get_shader_stages_size() = 0;
        
        
        const char* _name = nullptr;
        
    protected:
        
        VkDescriptorSetLayout _descriptor_set_layout =  VK_NULL_HANDLE;
        VkDescriptorPool      _descriptor_pool =        VK_NULL_HANDLE;
        VkDescriptorSet       _descriptor_set =         VK_NULL_HANDLE;
        
        //todo: check out the VkPhysicalDeviceLimits structure: https://vulkan.lunarg.com/doc/view/1.0.30.0/linux/vkspec.chunked/ch31s02.html
        static const int BINDING_MAX = 30;
        
        tsl::ordered_map<parameter_stage, resource::buffer_info>                    _uniform_buffers;
        tsl::ordered_map<parameter_stage, shader_parameter::shader_params_group>    _uniform_parameters;
        
        typedef tsl::ordered_map<const char*, resource::buffer_info>                buffer_parameter;
        tsl::ordered_map<parameter_stage, buffer_parameter>                         _sampler_buffers;
        
        typedef tsl::ordered_map< const char*, shader_parameter>                    sampler_parameter;
        tsl::ordered_map<parameter_stage, sampler_parameter>                        _sampler_parameters;
        
        std::array<VkDescriptorSetLayoutBinding, BINDING_MAX>                       _descriptor_set_layout_bindings;
        
        static const size_t MAX_SHADER_STAGES = 2;
        std::array<VkPipelineShaderStageCreateInfo, MAX_SHADER_STAGES>              _pipeline_shader_stages;
        
        bool _initialized = false;
        device* _device = nullptr;
    };
    
    using mat_shared_ptr = std::shared_ptr<material_base>;
}
