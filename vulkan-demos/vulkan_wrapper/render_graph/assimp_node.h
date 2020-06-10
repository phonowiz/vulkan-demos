//
//  diffuse_object.cpp
//  vulkan-demos
//
//  Created by Rafael Sabino on 6/5/20.
//  Copyright © 2020 Rafael Sabino. All rights reserved.
//

#pragma once


#include <filesystem>
#include "assimp_obj.h"

#include "device.h"
#include "node.h"

namespace vk
{

    template<uint32_t NUM_CHILDREN>
    class assimp_node : public vk::node<NUM_CHILDREN>
    {
    public:
        
        using node_type = vk::node<NUM_CHILDREN>;
         
        assimp_node(vk::device* dev, const char* path):
        _device(dev), _path(path)
        {
            node_type::_name = path;
        }
        
        assimp_node(vk::device* dev, const char* path, vk::vertex_components& v_components):
        _device(dev), _path(path)
        {
            node_type::_name = path;
            for(int i = 0; i < _mesh_lods.size(); ++i)
            {
              _mesh_lods[i].set_vertex_layout(v_components);
            }
        }
        
        vk::obj_shape* get_lod(uint32_t l)
        {
            if( l < _num_lods)
                return static_cast<vk::obj_shape*>(&_mesh_lods[l]);
            
            return static_cast<vk::obj_shape*>(&_mesh_lods[l-1]);
        }
        
        virtual void init_node() override
        {
            EA_ASSERT_MSG(_device != nullptr, "vk::device is nullptr");
            EA_ASSERT_MSG(_path != nullptr, "path directory is empty for this mesh");
            for( int i = 0; i < _mesh_lods.size(); ++i)
            {
                eastl::fixed_string<char, 250> name = _path;
                _mesh_lods[i].set_device(_device);
                _mesh_lods[i].set_path(name.c_str());
                if( i != 0)
                {
                    size_t p = name.find_last_of('.', name.length());
                    eastl::fixed_string<char, 250> lod_name;
                    lod_name = name.substr(0, p);
                    
                    //size_t p = name.find_last_of('.', name.length());
                    eastl::fixed_string<char, 250> lod_final = {};
                    
                    eastl::fixed_string<char, 250> extension = name.substr(p, name.length());
                    lod_final.sprintf("%s_lod%i%s", lod_name.c_str(), i, extension.c_str());
                    
                    if (!std::filesystem::exists(lod_name.c_str()))
                        break;
                    //name.sprintf("%s_lod%i", _path, i);
                    _mesh_lods[i].set_path(lod_name.c_str());
                    ++_num_lods;
                }
                _mesh_lods[i].create();
            }
        }
        
        void init_transforms(vk::transform& transform)
        {
            for( int image = 0; image < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++image)
            {
                transforms[image] = transform;
                transforms[image].update_transform_matrix();
            }
        }
        
        
        virtual void update_node(vk::camera& camera, uint32_t image_id) override
        {
            for( int i = 0; i < _mesh_lods.size(); ++i)
            {
                _mesh_lods[i].transform = transforms[image_id];
            }
        }
        
        virtual bool record_node_commands(command_recorder& buffer, uint32_t image_id) override { return true; }
        
        VkPipelineStageFlagBits get_producer_stage() override {  return VK_PIPELINE_STAGE_TRANSFER_BIT; };
        VkPipelineStageFlagBits get_consumer_stage() override {  return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT; };
        
        virtual void create_gpu_resources() override
        {}
        
        virtual void destroy() override
        {
            for( int i = 0; i < _mesh_lods.size(); ++i)
            {
                _mesh_lods[i].destroy();
            }
        }
        
        eastl::array<vk::transform, glfw_swapchain::NUM_SWAPCHAIN_IMAGES> transforms {} ;
        
        virtual char const * const * get_instance_type() override { return (&_node_type); };
        static char const * const *  get_class_type(){ return (&_node_type); }
        
    private:
        
        static constexpr char const * _node_type = nullptr;
        
        vk::device* _device = nullptr;
        eastl::array<vk::assimp_obj, 10> _mesh_lods;
        uint32_t    _num_lods = 1;
        const char* _path;
    };

}




