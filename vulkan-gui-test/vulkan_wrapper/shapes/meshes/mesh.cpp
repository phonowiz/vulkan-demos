//
//  mesh.cpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 2/11/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#include "vulkan_wrapper/shapes/meshes/mesh.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include "vertex.h"
#include "visual_material.h"

using namespace vk;

const std::string mesh::_mesh_resource_path =  "/models/";


mesh::mesh( device* device, tinyobj::attrib_t &vertex_attributes, tinyobj::shape_t& shape, tinyobj::material_t& material )
{
    _device = device;
    std::unordered_map<vertex, uint32_t> map_vertices;
    
    for(tinyobj::index_t index : shape.mesh.indices)
    {
        glm::vec3 pos(
                      vertex_attributes.vertices[3 * index.vertex_index + 0],
                      vertex_attributes.vertices[3 * index.vertex_index + 1],
                      vertex_attributes.vertices[3 * index.vertex_index + 2]
                      );
        
        glm::vec3 normal
        (
         vertex_attributes.normals[3 * index.normal_index + 0],
         vertex_attributes.normals[3 * index.normal_index + 1],
         vertex_attributes.normals[3 * index.normal_index + 2]
         );
        
        //TODO: you read the material informamtion but don't use it here, try getting the color from the materials variable.
        //if there is no material in obj, there needs to be a default material.  If a material color is assigned programatically, choose this one above
        //all other options.
        
        glm::vec4 color = glm::vec4(material.diffuse[0], material.diffuse[1], material.diffuse[2], 1.0f);
        vertex vert(pos, color, glm::vec2( 0.0f, 0.0f), normal);
        
        if(map_vertices.count(vert) == 0)
        {
            map_vertices[vert] = static_cast<uint32_t>(map_vertices.size());
            _vertices.push_back(vert);
        }
        
        _indices.push_back(map_vertices[vert]);
    }
    
    allocate_gpu_memory();
}

template<typename T>
void mesh::create_and_upload_buffer(VkCommandPool command_pool,
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

void mesh::draw(VkCommandBuffer commnad_buffer, vk::graphics_pipeline& pipeline)
{
    if(_active)
    {
        VkDeviceSize offsets[] = { 0 };
        assert(_vertex_buffer != nullptr);
        
        vkCmdBindVertexBuffers(commnad_buffer, 0, 1, &_vertex_buffer, offsets);
        vkCmdBindIndexBuffer(commnad_buffer, _index_buffer, 0, VK_INDEX_TYPE_UINT32);
        
        vkCmdBindDescriptorSets(commnad_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline._pipeline_layout, 0, 1, pipeline._material->get_descriptor_set(), 0, nullptr);
        
        vkCmdDrawIndexed(commnad_buffer, static_cast<uint32_t>(get_indices().size()), 1, 0, 0, 0);
    }
}

void mesh::create_vertex_buffer()
{
    create_and_upload_buffer(_device->_graphics_command_pool,
                          _vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, _vertex_buffer, _vertex_buffer_device_memory);
}

void mesh::create_index_buffer()
{
    create_and_upload_buffer( _device->_graphics_command_pool,
                          _indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, _index_buffer, _index_buffer_device_memory);
}

void mesh::allocate_gpu_memory()
{
    create_vertex_buffer();
    create_index_buffer();
}

void mesh::destroy()
{
    vkFreeMemory(_device->_logical_device, _index_buffer_device_memory, nullptr);
    vkDestroyBuffer(_device->_logical_device, _index_buffer, nullptr);
    
    _index_buffer_device_memory = VK_NULL_HANDLE;
    _index_buffer = VK_NULL_HANDLE;
    
    vkFreeMemory(_device->_logical_device, _vertex_buffer_device_memory, nullptr);
    vkDestroyBuffer(_device->_logical_device, _vertex_buffer, nullptr);
    
    _vertex_buffer_device_memory = VK_NULL_HANDLE;
    _vertex_buffer = VK_NULL_HANDLE;
}
mesh::~mesh()
{
}
