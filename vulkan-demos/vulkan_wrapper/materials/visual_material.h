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
#include "material_base.h"
#include <array>


#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


namespace  vk
{
    class visual_material;

    using visual_mat_shared_ptr = eastl::shared_ptr<visual_material>;

    class visual_material : public material_base
    {
    public:
        
        visual_material( visual_mat_shared_ptr& original)
        {
            operator=(*original);
        }
        visual_material(const char* name, shader_shared_ptr vertex_shader, shader_shared_ptr fragment_shader, device* device );

        virtual VkPipelineShaderStageCreateInfo* get_shader_stages() override
        {
            _pipeline_shader_stages[0] = _vertex_shader->_pipeline_shader_stage;
            _pipeline_shader_stages[1] = _fragment_shader->_pipeline_shader_stage;
            
            return _pipeline_shader_stages.data();
        }
        virtual size_t get_shader_stages_size() override { return 2; }
        
        virtual char const  * const * get_instance_type() override { return &_type; }
        static char const * const * get_material_type() { return &_type; }

        virtual void destroy() override;
        
        inline void init_parameter(const char* parameter_name, vk::parameter_stage stage, float value, int32_t binding)
        {
            get_uniform_parameters(stage, binding)[parameter_name] = value;
        };
        
        inline void init_parameter(const char* parameter_name, vk::parameter_stage stage, int32_t value, int32_t binding)
        {
            get_uniform_parameters(stage, binding)[parameter_name] = value;
        };
        
        inline void init_parameter(const char* parameter_name, vk::parameter_stage stage, uint32_t value, int32_t binding)
        {
            get_uniform_parameters(stage, binding)[parameter_name] = value;
        };
        
        inline void init_parameter(const char* parameter_name, vk::parameter_stage stage, glm::vec3 value, int32_t binding)
        {
            get_uniform_parameters(stage, binding)[parameter_name] = value;
        };
        inline void init_parameter(const char* parameter_name, vk::parameter_stage stage, glm::vec4 value, int32_t binding)
        {
            get_uniform_parameters(stage, binding)[parameter_name] = value;
        };
        inline void init_parameter(const char* parameter_name, vk::parameter_stage stage, glm::vec2 value, int32_t binding)
        {
            get_uniform_parameters(stage, binding)[parameter_name] = value;
        }
        inline void init_parameter(const char* parameter_name, vk::parameter_stage stage, glm::mat4 value, int32_t binding)
        {
            get_uniform_parameters(stage, binding)[parameter_name] = value;
        }
        
        inline void init_parameter(const char* parameter_name, vk::parameter_stage stage, glm::vec4* vecs, size_t num_vectors, int32_t binding)
        {
            get_uniform_parameters(stage, binding)[parameter_name].set_vectors_array(vecs, num_vectors);
        }
        
        inline void add_input_attachment( image* texture, const char* parameter_name, uint32_t attachment_id,
                                  vk::parameter_stage stage, uint32_t binding)
        {

            buffer_info& mem = _sampler_buffers[stage][parameter_name];
            mem.binding = binding;
            mem.usage_type = vk::usage_type::INPUT_ATTACHMENT;

            _sampler_parameters[stage][parameter_name] = texture;
        }

        inline visual_material& operator=( const visual_material& right)
        {
            if( this != &right)
            {
                this->material_base::operator=(static_cast<const material_base&>(right));
                
                _vertex_shader = right._vertex_shader;
                _fragment_shader = right._fragment_shader;
            }
            
            return *this;
        }
        
        //shader_parameter::shader_params_group& get_uniform_parameters(parameter_stage stage, uint32_t binding);
        visual_material::object_shader_params_group& get_dynamic_parameters(parameter_stage stage, uint32_t binding);
        
        ~visual_material();
    
    protected:
        
        shader_shared_ptr _vertex_shader = nullptr;
        shader_shared_ptr _fragment_shader = nullptr;
            
        static constexpr char const* const _type = nullptr;
    };
    

}
