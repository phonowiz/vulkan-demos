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
        
        inline void set_image_sampler(texture_3d& textures, const char* parameter_name, uint32_t binding, resource::usage_type usage)
        {
            //for( int i = 0; i < _material.size(); ++i)
            {
                _material[0]->set_image_sampler(&textures, parameter_name, material_base::parameter_stage::COMPUTE, binding, usage);
            }
        }

        void record_dispatch_commands(VkCommandBuffer&  command_buffer,
                                       uint32_t local_groups_in_x, uint32_t local_groups_in_y, uint32_t local_groups_in_z, uint32_t swapchain_index);
        
        inline void record_begin_commands(  std::function<void()> f){ _on_begin = f; };
        
        virtual void commit_parameters_to_gpu(uint32_t swapchain_id) override
        {
            _material[0]->commit_parameters_to_gpu();
        }
        
        bool is_initialized()
        {
            bool result = true;
            for( int i =0; i < _pipeline.size(); ++i)
            {
                if(_pipeline[i] == VK_NULL_HANDLE || _pipeline_layout[i] == VK_NULL_HANDLE)
                {
                    result = false;
                    break;
                }
            }
            return result;
        }
        
        virtual void destroy() override
        {
            for(int i = 0; i < _pipeline.size(); ++i)
            {
                vkDestroyPipeline(_device->_logical_device, _pipeline[i], nullptr);
                vkDestroyPipelineLayout(_device->_logical_device, _pipeline_layout[i], nullptr);
                
                _pipeline[i] = VK_NULL_HANDLE;
                _pipeline_layout[i] = VK_NULL_HANDLE;
            }
        }
        
        inline void commit_parameter_to_gpu(uint32_t swapchain_index) { _material[0]->commit_parameters_to_gpu(); }
        //LOCAL_GROUP_SIZE was chosen here because of an example I saw on the internet, if you decide to change this number
        //make sure the local group sizes in  your particular shader is changed as well. Or maybe this needs to be configurable by
        //the client
        static constexpr uint32_t LOCAL_GROUP_SIZE = 8u;
        
    private:
        
        std::array<VkPipeline, 1 >       _pipeline {};
        std::array<VkPipelineLayout, 1>  _pipeline_layout {};
        
        std::array<compute_mat_shared_ptr, 1> _material = {};
        
        std::function<void()> _on_begin = [](){};

        void create();
    };
};

