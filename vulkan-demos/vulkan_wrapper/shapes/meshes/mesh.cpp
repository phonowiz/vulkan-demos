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
#include "graphics_pipeline.h"

using namespace vk;

const eastl::string mesh::_mesh_resource_path =  "/models/";


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
