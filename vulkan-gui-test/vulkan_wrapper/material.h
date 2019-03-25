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

#include "device.h"
#include "shader.h"
#include "shader_parameter.h"
#include "tsl/ordered_map.h"
#include <array>


namespace  vk
{
    class material : public resource
    {
    public:
        material(const char* name, ShaderSharedPtr vertexShader, ShaderSharedPtr fragmentShader, device* device );
        
        //currently we oly support 3 shader stages max: vertex, pixel, and compute. geometry is not supported on macs.
        //compute pipeline is not yet implemented in this code.
        std::array<VkPipelineShaderStageCreateInfo,2> get_shader_stages()
        {
            std::array<VkPipelineShaderStageCreateInfo, 2> stages;
            stages[0] = _vertex_shader->_pipelineShaderStage;
            stages[1] = _fragment_shader->_pipelineShaderStage;
            
            return stages;
        }
        
        enum class parameter_stage
        {
            VERTEX = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT,
            FRAGMENT = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT,
            COMPUTE = VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT
        };
        
        void init_shader_parameters();
        void commit_parameters_to_gpu();
        
        //todo: maybe this should be called setupParameterBinding, because the layout describes the type of data that is
        //bound to the shader
        void create_descriptor_set_layout();
        void create_descriptor_pool();
        void create_descriptor_sets();
        void deallocate_parameters();
        virtual void destroy() override;
        
        inline VkDescriptorSetLayout* get_descriptor_set_layout(){ return &_descriptor_set_layout; }
        inline VkDescriptorSet* get_descriptor_set(){ return &_descriptor_set; }
        
        shader_parameter::shader_params_group& get_uniform_parameters(parameter_stage parameterStage, uint32_t binding);
        void set_image_sampler(texture_2d* texture, const char* parameterName, parameter_stage parameterStage, uint32_t binding);
        
        ShaderSharedPtr _vertex_shader;
        ShaderSharedPtr _fragment_shader;
        
        
        VkDescriptorSetLayout _descriptor_set_layout = VK_NULL_HANDLE;
        VkDescriptorPool      _descriptor_pool = VK_NULL_HANDLE;
        VkDescriptorSet       _descriptor_set = VK_NULL_HANDLE;
        
        //todo: check out the VkPhysicalDeviceLimits structure: https://vulkan.lunarg.com/doc/view/1.0.30.0/linux/vkspec.chunked/ch31s02.html
        static const int BINDING_MAX = 30;
        
        
        tsl::ordered_map<parameter_stage, resource::buffer_info>                  _uniform_buffers;
        tsl::ordered_map<parameter_stage, shader_parameter::shader_params_group>    _uniform_parameters;


        tsl::ordered_map<parameter_stage, resource::buffer_info>                  _sampler_buffers;
        
        typedef tsl::ordered_map< const char*, shader_parameter>                  sampler_parameter;
        
        tsl::ordered_map<parameter_stage, sampler_parameter>                     _sampler_parameters;
        std::array<VkDescriptorSetLayoutBinding, BINDING_MAX>                    _descriptor_set_layout_bindings;
        
        
        const char* _name = nullptr;
        
        device *_device = nullptr;
        
        bool initialized = false;
        ~material();
    
    private:
        
    };
    
    
    using material_shared_ptr = std::shared_ptr<material>;
}
