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
#include "material.h"


class Mesh
{
private:
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    
    VkBuffer _vertexBuffer = VK_NULL_HANDLE;
    VkBuffer _indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
    
    
public:
    Mesh()
    {};
    
    void create(const char* path);
    
    template<typename T>
    inline void createAndUploadBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkQueue queue, VkCommandPool commandPool,
                                      std::vector<T>& data, VkBufferUsageFlags usage, VkBuffer &buffer, VkDeviceMemory &deviceMemory);
    
    void createBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize deviceSize, VkBufferUsageFlags bufferUsageFlags, VkBuffer &buffer,
                      VkMemoryPropertyFlags memoryPropertyFlags, VkDeviceMemory &deviceMemory);
    
    void draw(VkCommandBuffer commandBuffer, VkPipelineLayout pilineLayout, vk::MaterialSharedPtr material);
    
    std::vector<Vertex>& getVertices()
    {
        return vertices;
    }
    std::vector<uint32_t>& getIndices()
    {
        return indices;
    }
    
};
