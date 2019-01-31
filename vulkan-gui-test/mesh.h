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
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"


class Mesh
{
private:
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    
    
public:
    Mesh()
    {
        
    }
    
    void create(const char* path)
    {
        tinyobj::attrib_t vertexAttributes;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        
        std::string errorString;
        std::string warnString;
        
    /*
     bool LoadObj(attrib_t *attrib, std::vector<shape_t> *shapes,
     std::vector<material_t> *materials, std::string *warn,
     std::string *err, const char *filename, const char *mtl_basedir,
     bool trianglulate, bool default_vcols_fallback)
     
     */
        bool success = tinyobj::LoadObj(&vertexAttributes, &shapes, &materials, &warnString, &errorString, path);
    
        
        assert(success && "check errorString variable");
        
        //TODO: YOU NEED TO USE AN UNORDERED MAP TO ELIMINATE DUPLICATE VERTICES
        //check the end of this tutorial: https://www.youtube.com/watch?v=KedrqATjoy0&list=PL58qjcU5nk8uH9mmlASm4SFy1yuPzDAH0&index=106
        for(tinyobj::shape_t shape:  shapes)
        {
            for(tinyobj::index_t index : shape.mesh.indices)
            {
                glm::vec3 pos = {
                    vertexAttributes.vertices[3 * index.vertex_index + 0],
                    vertexAttributes.vertices[3 * index.vertex_index + 1],
                    vertexAttributes.vertices[3 * index.vertex_index + 2]
                };
                
                glm::vec3 normal =
                {
                    vertexAttributes.normals[3 * index.normal_index + 0],
                    vertexAttributes.normals[3 * index.normal_index + 1],
                    vertexAttributes.normals[3 * index.normal_index + 2]
                };
                Vertex vert(pos, glm::vec3{ 0.0f, 1.0f, 0.0f }, glm::vec2{ 0, 0}, normal);
                
                vertices.push_back(vert);
                indices.push_back(indices.size());
            }
        }
    }
    std::vector<Vertex>& getVertices()
    {
        return vertices;
    }
    std::vector<uint32_t>& getIndices()
    {
        return indices;
    }
    
};
