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
        
        compute_pipeline(){}
        
        compute_pipeline(device* device, compute_mat_shared_ptr material) :
        pipeline(device)
        {
            set_material(material);
        }
        
        inline void set_material(compute_mat_shared_ptr mat)
        {
            for(int i =0; i < _material.size(); ++i)
            {
                _material[i] = mat;
            }
            
        }
        
        inline void set_image_sampler(std::array<texture_3d, glfw_swapchain::NUM_SWAPCHAIN_IMAGES>& textures, const char* parameter_name, uint32_t binding, resource::usage_type usage)
        {
            for( int i = 0; i < _material.size(); ++i)
            {
                _material[i]->set_image_sampler(&textures[i], parameter_name, material_base::parameter_stage::COMPUTE, binding, usage);
            }
        }

        void record_dispatch_commands(VkCommandBuffer&  command_buffer,
                                       uint32_t local_groups_in_x, uint32_t local_groups_in_y, uint32_t local_groups_in_z, uint32_t swapchain_index);
        
        inline void record_begin_commands(  std::function<void()> f){ _on_begin = f; };
        
        virtual void commit_parameters_to_gpu(uint32_t swapchain_id) override
        {
            _material[swapchain_id]->commit_parameters_to_gpu();
        }
        
        inline void commit_parameter_to_gpu(uint32_t swapchain_index) { _material[swapchain_index]->commit_parameters_to_gpu(); }
        //LOCAL_GROUP_SIZE was chosen here because of an example I saw on the internet, if you decide to change this number
        //make sure the local group sizes in  your particular shader is changed as well. Or maybe this needs to be configurable by
        //the client
        static constexpr uint32_t LOCAL_GROUP_SIZE = 8u;
        
    private:
        
        std::array<compute_mat_shared_ptr, glfw_swapchain::NUM_SWAPCHAIN_IMAGES> _material = {};
        
        std::function<void()> _on_begin = [](){};

        void create();
    };
};

