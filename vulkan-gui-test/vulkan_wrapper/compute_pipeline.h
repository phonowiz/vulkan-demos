//
//  compute_pipeline.hpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 6/29/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#pragma once

#include "pipeline.h"
#include "compute_material.h"

namespace vk
{
    class device;
    
    class compute_pipeline : public pipeline
    {
    public:
        
        compute_pipeline(device* device, compute_mat_shared_ptr material) :
        pipeline(device),
        _material(material)
        {
        }
        
        
        inline void set_image_sampler(texture_2d* texture, const char* parameter_name, uint32_t binding)
        {
            assert(_material != nullptr && " no material assigned to this compute pipeline");
            _material->set_image_sampler(texture, parameter_name, material_base::parameter_stage::COMPUTE, binding);
        }
        
        compute_mat_shared_ptr _material = nullptr;
        
        void create();
        
    private:
    };
};

