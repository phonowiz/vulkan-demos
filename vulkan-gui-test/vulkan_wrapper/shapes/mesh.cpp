//
//  mesh.cpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 2/11/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#include "vulkan_wrapper/shapes/mesh.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include "vertex.h"
#include "visual_material.h"

using namespace vk;

const std::string mesh::_mesh_resource_path =  "/models/";

void mesh::create(const char* path)
{
    tinyobj::attrib_t vertexAttributes;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    
    std::string errorString;
    std::string warnString;
    
    std::string  fullPath = resource::resource_root + mesh::_mesh_resource_path + path;
    
    bool success = tinyobj::LoadObj(&vertexAttributes, &shapes, &materials, &warnString, &errorString, fullPath.c_str());
    
    assert(success && "check errorString variable");
    
    
    std::unordered_map<vertex, uint32_t> map_vertices;
    for(tinyobj::shape_t shape:  shapes)
    {
        for(tinyobj::index_t index : shape.mesh.indices)
        {
            glm::vec3 pos(
                          vertexAttributes.vertices[3 * index.vertex_index + 0],
                          vertexAttributes.vertices[3 * index.vertex_index + 1],
                          vertexAttributes.vertices[3 * index.vertex_index + 2]
                          );
            
            glm::vec3 normal
            (
             vertexAttributes.normals[3 * index.normal_index + 0],
             vertexAttributes.normals[3 * index.normal_index + 1],
             vertexAttributes.normals[3 * index.normal_index + 2]
             );
            vertex vert(pos, glm::vec3( 0.0f, 1.0f, 0.0f ), glm::vec2( 0.0f, 0.0f), normal);
            
            if(map_vertices.count(vert) == 0)
            {
                map_vertices[vert] = static_cast<uint32_t>(map_vertices.size());
                _vertices.push_back(vert);
            }
            
            _indices.push_back(map_vertices[vert]);
        }
    }
    
    allocate_gpu_memory();
}

template<typename T>
void mesh::create_and_upload_buffer(VkCommandPool commandPool,
                                  std::vector<T>& data, VkBufferUsageFlags usage, VkBuffer &buffer, VkDeviceMemory &deviceMemory)
{
    VkDeviceSize bufferSize = sizeof(T) * data.size();
    assert(data.size() != 0);
    VkBuffer stagingBuffer;
    VkDeviceMemory statingBufferMemory;
    
    create_buffer(_device->_logical_device, _device->_physical_device, bufferSize,  VK_BUFFER_USAGE_TRANSFER_SRC_BIT, stagingBuffer,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, statingBufferMemory);
    
    void* rawData= nullptr;
    VkResult r = vkMapMemory(_device->_logical_device, statingBufferMemory, 0, bufferSize, 0, &rawData);
    ASSERT_VULKAN(r);
    memcpy(rawData, data.data(), bufferSize);
    vkUnmapMemory(_device->_logical_device, statingBufferMemory);
    
    
    create_buffer(_device->_logical_device, _device->_physical_device, bufferSize, usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT, buffer,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, deviceMemory);
    
    _device->copy_buffer(commandPool, _device->_graphics_queue, stagingBuffer, buffer, bufferSize);
    vkDestroyBuffer(_device->_logical_device, stagingBuffer, nullptr);
    vkFreeMemory(_device->_logical_device, statingBufferMemory, nullptr);
    
}

void mesh::draw(VkCommandBuffer commandBuffer, vk::graphics_pipeline& pipeline)
{
    VkDeviceSize offsets[] = { 0 };
    assert(_vertex_buffer != nullptr);
    
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &_vertex_buffer, offsets);
    vkCmdBindIndexBuffer(commandBuffer, _index_buffer, 0, VK_INDEX_TYPE_UINT32);
    
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline._pipeline_layout, 0, 1, pipeline._material->get_descriptor_set(), 0, nullptr);
    
    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(get_indices().size()), 1, 0, 0, 0);
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
