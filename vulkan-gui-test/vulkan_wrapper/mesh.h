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
#include "pipeline.h"


//todo: move this class to  vk namespace


namespace vk
{
    
    class mesh : resource
    {
    protected:
        std::vector<vertex>   _vertices;
        std::vector<uint32_t> _indices;
        device* _device = nullptr;
        
        VkBuffer        _vertexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory  _vertexBufferDeviceMemory = VK_NULL_HANDLE;
        VkBuffer        _indexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory  _indexBufferDeviceMemory = VK_NULL_HANDLE;
        
    protected:
        mesh(){};
    public:
        
        mesh(const char* path, device* device)
        {
            create(path);
            _device = device;
        };
        
        ~mesh();
        void create(const char* path);
        
        template<typename T>
        inline void createAndUploadBuffer( VkCommandPool commandPool,
                                          std::vector<T>& data, VkBufferUsageFlags usage, VkBuffer &buffer, VkDeviceMemory &deviceMemory);
        
        
        static const std::string meshResourcePath;
        
        //public
        void draw(VkCommandBuffer commandBuffer, vk::pipeline& pipeline);
        
        //public
        void allocateGPUMemory();
        virtual void destroy() override;
        
        //private
        void createVertexBuffer();
        void createIndexBuffer();
        
        std::vector<vertex>& getVertices()
        {
            return _vertices;
        }
        std::vector<uint32_t>& getIndices()
        {
            return _indices;
        }
        
    };

}
