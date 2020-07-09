//
//  vertex.hpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 3/23/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#pragma once


#include <glm/glm.hpp>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include "EASTL/array.h"

#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif

#include <glm/gtx/hash.hpp>

namespace vk {
    
    class vertex
    {
    public:
        glm::vec3 _pos;
        glm::vec4 _color;
        glm::vec2 _uv_coord;
        glm::vec3 _normal;
        glm::vec3 _tangent;
        glm::vec3 _bitangent;
        
        
        vertex(glm::vec3 pos, glm::vec4 color, glm::vec2 uvCoord, glm::vec3 normal)
        : _pos(pos), _color(color), _uv_coord(uvCoord), _normal(normal)
        {}
        
        bool operator==(const vertex& other) const
        {
            return _pos == other._pos && _color == other._color && _uv_coord == other._uv_coord &&
            _normal == other._normal;
        }
        
        static VkVertexInputBindingDescription get_binding_description()
        {
            VkVertexInputBindingDescription vertex_input_binding_description;
            vertex_input_binding_description.binding = 0;
            vertex_input_binding_description.stride = sizeof(vertex);
            vertex_input_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            
            return vertex_input_binding_description;
        }
        
        static eastl::array<VkVertexInputAttributeDescription, 6> get_attribute_descriptions()
        {
            eastl::array<VkVertexInputAttributeDescription, 6> vertex_input_attributes_description {};
            
            vertex_input_attributes_description[0].location = 0;
            vertex_input_attributes_description[0].binding = 0;
            vertex_input_attributes_description[0].format = VK_FORMAT_R32G32B32_SFLOAT;
            vertex_input_attributes_description[0].offset = offsetof(vertex, _pos);
            
            vertex_input_attributes_description[1].location = 1;
            vertex_input_attributes_description[1].binding = 0;
            vertex_input_attributes_description[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
            vertex_input_attributes_description[1].offset = offsetof(vertex, _color);
            
            vertex_input_attributes_description[2].location = 2;
            vertex_input_attributes_description[2].binding = 0;
            vertex_input_attributes_description[2].format = VK_FORMAT_R32G32_SFLOAT;
            vertex_input_attributes_description[2].offset = offsetof(vertex, _uv_coord);
            
            vertex_input_attributes_description[3].location = 3;
            vertex_input_attributes_description[3].binding = 0;
            vertex_input_attributes_description[3].format = VK_FORMAT_R32G32B32_SFLOAT;
            vertex_input_attributes_description[3].offset = offsetof(vertex, _normal);
            
            vertex_input_attributes_description[4].location = 4;
            vertex_input_attributes_description[4].binding = 0;
            vertex_input_attributes_description[4].format = VK_FORMAT_R32G32B32_SFLOAT;
            vertex_input_attributes_description[4].offset = offsetof(vertex, _tangent);
            
            
            vertex_input_attributes_description[5].location = 5;
            vertex_input_attributes_description[5].binding = 0;
            vertex_input_attributes_description[5].format = VK_FORMAT_R32G32B32_SFLOAT;
            vertex_input_attributes_description[5].offset = offsetof(vertex, _bitangent);
            
            
            return vertex_input_attributes_description;
            
        }
    };
}


namespace std
{
    template<> struct hash<vk::vertex>
    {
        size_t operator()(vk::vertex const &vert) const
        {
            size_t h1 = hash<glm::vec3>()(vert._pos);
            size_t h2 = hash<glm::vec3>()(vert._color);
            size_t h3 = hash<glm::vec2>()(vert._uv_coord);
            
            return ((h1 ^ (h2 << 1)) >> 1) ^ h3;
        }
    };
}

