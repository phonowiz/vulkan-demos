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
#include "resource_set.h"
#include "material_store.h"


namespace vk
{
    class device;
    
    template<uint32_t NUM_MATERIALS = vk::NUM_SWAPCHAIN_IMAGES>
    class compute_pipeline : public pipeline
    {
    public:
        
        compute_pipeline(){}
        
        compute_pipeline(device* device, compute_mat_shared_ptr material) :
        pipeline(device)
        {
            //set_material(material);
        }
        
        inline void set_material(const char* name, material_store& store)
        {
            for(int i = 0; i < NUM_MATERIALS; ++i)
            {
                _material[i] = store.GET_MAT<vk::compute_material>(name);
            }
            
        }
        
        template<typename T>
        inline void set_image_sampler(resource_set<T>& textures, const char* parameter_name, uint32_t binding)
        {
            for( int i = 0; i < textures.size(); ++i)
            {
                //note: here we force STORAGE_IMAGE usage because the validation layers will throw errors if you use anything else
                _material[i]->set_image_sampler(&textures[i], parameter_name, material_base::parameter_stage::COMPUTE, binding, material_base::usage_type::STORAGE_IMAGE);
            }
        }
        
//        inline void set_image_sampler(resource_set<depth_texture>& textures, const char* parameter_name, uint32_t binding)
//        {
//            for( int i = 0; i < NUM_MATERIALS; ++i)
//            {
//                //note: here we force STORAGE_IMAGE usage because the validation layers will throw errors if you use anything else
//                _material[i]->set_image_sampler(&textures[i], parameter_name, material_base::parameter_stage::COMPUTE, binding, material_base::usage_type::STORAGE_IMAGE);
//            }
//        }
//
//
//        inline void set_image_sampler(resource_set<texture_2d>& textures, const char* parameter_name, uint32_t binding)
//        {
//            for( int i = 0; i < NUM_MATERIALS; ++i)
//            {
//                //note: here we force STORAGE_IMAGE usage because the validation layers will throw errors if you use anything else
//                _material[i]->set_image_sampler(&textures[i], parameter_name, material_base::parameter_stage::COMPUTE, binding, material_base::usage_type::STORAGE_IMAGE);
//            }
//        }
//
//        inline void set_image_sampler(resource_set<texture_3d>& textures, const char* parameter_name, uint32_t binding)
//        {
//            for( int i = 0; i < NUM_MATERIALS; ++i)
//            {
//                //note: here we force STORAGE_IMAGE usage because the validation layers will throw errors if you use anything else
//                _material[i]->set_image_sampler(&textures[i], parameter_name, material_base::parameter_stage::COMPUTE, binding, material_base::usage_type::STORAGE_IMAGE);
//            }
//        }
        
        inline void set_image_sampler(texture_3d& textures, const char* parameter_name, uint32_t binding)
        {
            for( int i = 0; i < NUM_MATERIALS; ++i )
            {
                //note: here we force STORAGE_IMAGE usage because the validation layers will throw errors if you use anything else
                _material[i]->set_image_sampler(&textures, parameter_name, material_base::parameter_stage::COMPUTE, binding, material_base::usage_type::STORAGE_IMAGE);
            }
        }

        void record_dispatch_commands(VkCommandBuffer&  command_buffer, uint32_t image_id,
                                       uint32_t local_groups_in_x, uint32_t local_groups_in_y, uint32_t local_groups_in_z);
        
        //TODO: THIS SHOULD GO AWAY...
        inline void record_begin_commands(  std::function<void()> f){ _on_begin = f; };
        
//        void commit_parameters_to_gpu(uint32_t i)
//        {
//            for( int i = 0; i < NUM_MATERIALS; ++i)
//                _material[i]->commit_parameters_to_gpu();
//        }
        
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
                _material[i]->destroy();
            }

        }
        
        inline void commit_parameter_to_gpu(uint32_t image_id) {
            
            _material[image_id]->commit_parameters_to_gpu();
        }

        //LOCAL_GROUP_SIZE was chosen here because of an example I saw on the internet, if you decide to change this number
        //make sure the local group sizes in  your particular shader is changed as well. Or maybe this needs to be configurable by
        //the client
        static constexpr uint32_t LOCAL_GROUP_SIZE = 8u;
        
    private:
        
        //TODO:: ARE THESE TWO ANY USE?????
        std::array<VkPipeline, NUM_MATERIALS >       _pipeline {};
        std::array<VkPipelineLayout, NUM_MATERIALS>  _pipeline_layout {};
        
        std::array<compute_mat_shared_ptr, NUM_MATERIALS> _material = {};
        
        std::function<void()> _on_begin = [](){};

        void create();
    };

    #include "compute_pipeline.hpp"

};

