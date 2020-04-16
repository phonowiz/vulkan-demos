//
//  compute_material.hpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 7/1/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#pragma once

#include "material_base.h"
#include "compute_material.h"
#include "shader.h"

namespace vk
{
    class compute_material : public material_base
    {
    public:
        compute_material(){};
        
        compute_material(compute_material& original)
        {
            operator=(original);
        }
        compute_material(const char* name, shader_shared_ptr compute_shader, device* device):
        material_base(device, name)
        {
            _compute_shader = compute_shader;
        };
        
        virtual VkPipelineShaderStageCreateInfo* get_shader_stages() override
        {
            _pipeline_shader_stages[0] = _compute_shader->_pipeline_shader_stage;
            
            return _pipeline_shader_stages.data();
        }
        
        virtual char const * const * get_instance_type() override { return &_type; }
        static char const* const * get_material_type() { return &_type; }
        
        virtual size_t get_shader_stages_size() override { return 1; }
        
        inline compute_material& operator=( const compute_material& right)
        {
            if( this != &right)
            {
                material_base::operator=(static_cast<const material_base&>(right));
                _compute_shader = right._compute_shader;
            }
            
            return *this;
        }
        
    private:
        static constexpr char const* const _type = nullptr;
        shader_shared_ptr _compute_shader;
    };

    using compute_mat_shared_ptr = std::shared_ptr<compute_material>;
};
