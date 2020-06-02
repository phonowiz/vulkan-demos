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
    template< uint32_t NUM_ATTACHMENTS>
    class render_pass;

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
        mesh(device* dev){_device = dev;}
    public:
        
        struct material_properties
        {
            glm::vec4 color = glm::vec4(1.0f);
        };
        
        mesh( device* device, tinyobj::attrib_t &vertex_attributes, tinyobj::shape_t& shape, tinyobj::material_t& material );
        
        ~mesh();
        
        template<typename T>
        void create_and_upload_buffer(VkCommandPool command_pool,
                                          std::vector<T>& data, VkBufferUsageFlags usage, VkBuffer &buffer, VkDeviceMemory &device_memory)
        {
            VkDeviceSize buffer_size = sizeof(T) * data.size();
            assert(data.size() != 0);
            VkBuffer staging_buffer {};
            VkDeviceMemory staging_buffer_memory {};
            
            create_buffer(_device->_logical_device, _device->_physical_device, buffer_size,  VK_BUFFER_USAGE_TRANSFER_SRC_BIT, staging_buffer,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer_memory);
            
            void* raw_data= nullptr;
            VkResult r = vkMapMemory(_device->_logical_device, staging_buffer_memory, 0, buffer_size, 0, &raw_data);
            ASSERT_VULKAN(r);
            memcpy(raw_data, data.data(), buffer_size);
            vkUnmapMemory(_device->_logical_device, staging_buffer_memory);
            
            
            create_buffer(_device->_logical_device, _device->_physical_device, buffer_size, usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT, buffer,
                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, device_memory);
            
            _device->copy_buffer(command_pool, _device->_graphics_queue, staging_buffer, buffer, buffer_size);
            vkDestroyBuffer(_device->_logical_device, staging_buffer, nullptr);
            vkFreeMemory(_device->_logical_device, staging_buffer_memory, nullptr);
            
        }
        
        template<typename T>
        void create_and_upload_buffer_void(VkCommandPool command_pool,
                                          eastl::vector<T> &data, VkBufferUsageFlags usage, VkBuffer &buffer, VkDeviceMemory &device_memory)
        {
            VkDeviceSize buffer_size = sizeof(T) * data.size();
            assert(data.size() != 0);
            VkBuffer staging_buffer {};
            VkDeviceMemory staging_buffer_memory {};
            
            create_buffer(_device->_logical_device, _device->_physical_device, buffer_size,  VK_BUFFER_USAGE_TRANSFER_SRC_BIT, staging_buffer,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer_memory);
  
            void* raw_data= nullptr;
            VkResult r = vkMapMemory(_device->_logical_device, staging_buffer_memory, 0, buffer_size, 0, &raw_data);
            ASSERT_VULKAN(r);
            memcpy(raw_data, data.data(), buffer_size);
            vkUnmapMemory(_device->_logical_device, staging_buffer_memory);
            
            
            create_buffer(_device->_logical_device, _device->_physical_device, buffer_size, usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT, buffer,
                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, device_memory);
            
            _device->copy_buffer(command_pool, _device->_graphics_queue, staging_buffer, buffer, buffer_size);
            vkDestroyBuffer(_device->_logical_device, staging_buffer, nullptr);
            vkFreeMemory(_device->_logical_device, staging_buffer_memory, nullptr);
            
        }

        static const eastl::string _mesh_resource_path;
        
        template<uint32_t NUM_ATTACHMENTS>
        void draw(VkCommandBuffer command_buffer, vk::render_pass< NUM_ATTACHMENTS>& pipeline, uint32_t object_index, uint32_t swapchain_index);
        
        virtual void destroy() override;
        
        inline void bind_verteces(VkCommandBuffer& command_buffer)
        {
            VkDeviceSize offsets[] = { 0 };
            assert(_vertex_buffer != nullptr);

            vkCmdBindVertexBuffers(command_buffer, 0, 1, &_vertex_buffer, offsets);
            vkCmdBindIndexBuffer(command_buffer, _index_buffer, 0, VK_INDEX_TYPE_UINT32);
        }
        
        virtual void draw_indexed(VkCommandBuffer command_buffer, uint32_t instance_count)
        {
            vkCmdDrawIndexed(command_buffer, static_cast<uint32_t>(get_indices().size()), instance_count, 0, 0, 0);
        }
        virtual void draw(VkCommandBuffer command_buffer)
        {
            vkCmdDraw(command_buffer, static_cast<uint32_t>(_vertices.size()), 1, 0,0 );
        }
    
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
