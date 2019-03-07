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


//todo: move this class to  vk namespace


namespace vk
{
    
    class Mesh : Resource
    {
    private:
        std::vector<Vertex>   _vertices;
        std::vector<uint32_t> _indices;
        PhysicalDevice* _device = nullptr;
        
        VkBuffer        _vertexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory  _vertexBufferDeviceMemory = VK_NULL_HANDLE;
        VkBuffer        _indexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory  _indexBufferDeviceMemory = VK_NULL_HANDLE;
        
    public:
        
        Mesh(const char* path, PhysicalDevice* device)
        {
            create(path);
            _device = device;
        };
        
        ~Mesh();
        void create(const char* path);
        
        template<typename T>
        inline void createAndUploadBuffer( VkCommandPool commandPool,
                                          std::vector<T>& data, VkBufferUsageFlags usage, VkBuffer &buffer, VkDeviceMemory &deviceMemory);
        
//        void createBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize deviceSize, VkBufferUsageFlags bufferUsageFlags, VkBuffer &buffer,
//                          VkMemoryPropertyFlags memoryPropertyFlags, VkDeviceMemory &deviceMemory);
        
        static const std::string meshResourcePath;
        
        //public
        void draw(VkCommandBuffer commandBuffer, VkPipelineLayout pilineLayout, MaterialSharedPtr material);
        
        //public
        void allocateGPUMemory(VkCommandPool commandPool);
        virtual void destroy() override;
        
        //private
        void createVertexBuffer(VkCommandPool commandPool);
        void createIndexBuffer(VkCommandPool commandPool);
        
        std::vector<Vertex>& getVertices()
        {
            return _vertices;
        }
        std::vector<uint32_t>& getIndices()
        {
            return _indices;
        }
        
    };

}
