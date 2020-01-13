//
//  mesh.h
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 1/30/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#pragma once

#include <vector>
#include <string>
#include <assert.h>
#include <algorithm>
#include <unordered_map>

#include "vertex.h"
#include "visual_material.h"
#include "compute_pipeline.h"

#include "tiny_obj_loader.h"

namespace vk
{
    class graphics_pipeline;

    class mesh : resource
    {
    protected:
        std::vector<vertex>   _vertices;
        std::vector<uint32_t> _indices;
        device* _device = nullptr;
        
        VkBuffer        _vertex_buffer = VK_NULL_HANDLE;
        VkDeviceMemory  _vertex_buffer_device_memory = VK_NULL_HANDLE;
        VkBuffer        _index_buffer = VK_NULL_HANDLE;
        VkDeviceMemory  _index_buffer_device_memory = VK_NULL_HANDLE;
        
    protected:
        mesh(){};
    public:
        
        struct material_properties
        {
            glm::vec4 color = glm::vec4(1.0f);
        };
        
        mesh( device* device, tinyobj::attrib_t &vertex_attributes, tinyobj::shape_t& shape, tinyobj::material_t& material );
        
        ~mesh();
        
        template<typename T>
        inline void create_and_upload_buffer( VkCommandPool commandPool,
                                          std::vector<T>& data, VkBufferUsageFlags usage, VkBuffer &buffer, VkDeviceMemory &deviceMemory);
        
        static const std::string _mesh_resource_path;
        
        void draw(VkCommandBuffer command_buffer, vk::graphics_pipeline& pipeline, uint32_t object_index, uint32_t swapchain_index);
        
        virtual void destroy() override;
    
        inline std::vector<vertex>& get_vertices()
        {
            return _vertices;
        }
        inline std::vector<uint32_t>& get_indices()
        {
            return _indices;
        }
        
    protected:
        
        bool _active = true;
        void create_vertex_buffer();
        void create_index_buffer();
        void allocate_gpu_memory();
    };

}
