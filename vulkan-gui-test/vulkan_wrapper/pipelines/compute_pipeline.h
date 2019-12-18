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
#include "resource.h"

namespace vk
{
    class device;
    
    class compute_pipeline : public pipeline
    {
    public:
        
        compute_pipeline(device* device, compute_mat_shared_ptr material) :
        pipeline(device),
        material(material)
        {
        }
        
        inline void set_image_sampler(texture_2d* texture, const char* parameter_name, uint32_t binding, resource::usage_type usage)
        {
            assert(material != nullptr && " no material assigned to this compute pipeline");
            material->set_image_sampler(texture, parameter_name, material_base::parameter_stage::COMPUTE, binding, usage);
        }
        
        inline void set_image_sampler(texture_3d* texture, const char* parameter_name, uint32_t binding, resource::usage_type usage)
        {
            assert(material != nullptr && " no material assigned to this compute pipeline");
            material->set_image_sampler(texture, parameter_name, material_base::parameter_stage::COMPUTE, binding, usage);
        }
        
        compute_mat_shared_ptr material = nullptr;
        

        void record_dispatch_commands(VkCommandBuffer&  command_buffer,
                                       uint32_t local_groups_in_x, uint32_t local_groups_in_y, uint32_t local_groups_in_z);
        
        inline void record_begin_commands(  std::function<void()> f){ _on_begin = f; };
        
        //LOCAL_GROUP_SIZE was chosen here because of an example I saw on the internet, if you decide to change this number
        //make sure the local group sizes in  your particular shader is changed as well. Or maybe this needs to be configurable by
        //the client
        static constexpr uint32_t LOCAL_GROUP_SIZE = 8u;
        
    private:
        
        std::function<void()> _on_begin = [](){};
        
        void create();
    };
};

