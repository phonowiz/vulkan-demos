//
//  mesh.cpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 2/11/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#include "mesh.h"

//#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include "material.h"



void Mesh::create(const char* path)
{
    tinyobj::attrib_t vertexAttributes;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    
    std::string errorString;
    std::string warnString;
    
    bool success = tinyobj::LoadObj(&vertexAttributes, &shapes, &materials, &warnString, &errorString, path);
    
    
    assert(success && "check errorString variable");
    
    
    std::unordered_map<Vertex, uint32_t> map_vertices;
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
            Vertex vert(pos, glm::vec3( 0.0f, 1.0f, 0.0f ), glm::vec2( 0.0f, 0.0f), normal);
            
            if(map_vertices.count(vert) == 0)
            {
                map_vertices[vert] = static_cast<uint32_t>(map_vertices.size());
                vertices.push_back(vert);
            }
            
            indices.push_back(map_vertices[vert]);
        }
    }
}

template<typename T>
void Mesh::createAndUploadBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkQueue queue, VkCommandPool commandPool,
                                 std::vector<T>& data, VkBufferUsageFlags usage, VkBuffer &buffer, VkDeviceMemory &deviceMemory)
{
    VkDeviceSize bufferSize = sizeof(T) * data.size();
    assert(data.size() != 0);
    VkBuffer stagingBuffer;
    VkDeviceMemory statingBufferMemory;
    
    createBuffer(device, physicalDevice, bufferSize,  VK_BUFFER_USAGE_TRANSFER_SRC_BIT, stagingBuffer,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, statingBufferMemory);
    
    void* rawData= nullptr;
    VkResult r = vkMapMemory(device, statingBufferMemory, 0, bufferSize, 0, &rawData);
    ASSERT_VULKAN(r);
    memcpy(rawData, data.data(), bufferSize);
    vkUnmapMemory(device, statingBufferMemory);
    
    
    createBuffer(device, physicalDevice, bufferSize, usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT, buffer,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, deviceMemory);
    
    copyBuffer(device, commandPool, queue, stagingBuffer, buffer, bufferSize);
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, statingBufferMemory, nullptr);
    
}

void Mesh::createBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize deviceSize, VkBufferUsageFlags bufferUsageFlags, VkBuffer &buffer,
                        VkMemoryPropertyFlags memoryPropertyFlags, VkDeviceMemory &deviceMemory)
{
    VkBufferCreateInfo bufferCreateInfo;
    
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.pNext = nullptr;
    bufferCreateInfo.flags = 0;
    bufferCreateInfo.size = deviceSize;
    bufferCreateInfo.usage = bufferUsageFlags;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferCreateInfo.queueFamilyIndexCount = 0;
    bufferCreateInfo.pQueueFamilyIndices = nullptr;
    
    VkResult result = vkCreateBuffer(device, &bufferCreateInfo, nullptr, &buffer);
    ASSERT_VULKAN(result);
    VkMemoryRequirements memoryRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memoryRequirements);
    
    VkMemoryAllocateInfo memoryAllocateInfo;
    memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocateInfo.pNext = nullptr;
    memoryAllocateInfo.allocationSize = memoryRequirements.size;
    memoryAllocateInfo.memoryTypeIndex = findMemoryTypeIndex(physicalDevice,memoryRequirements.memoryTypeBits, memoryPropertyFlags);
    
    result = vkAllocateMemory(device, &memoryAllocateInfo, nullptr, &deviceMemory);
    ASSERT_VULKAN(result);
    vkBindBufferMemory(device, buffer, deviceMemory, 0);
    
}

void Mesh::draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, vk::MaterialSharedPtr material)
{
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &_vertexBuffer, offsets);
    vkCmdBindIndexBuffer(commandBuffer, _indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, material->getDescriptorSet(), 0, nullptr);
    
    //in the render target, there will be list of registered meshes that will submit their indices
    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(getIndices().size()), 1, 0, 0, 0);
}
