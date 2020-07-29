//
//  assimp_loader.hpp
//  vulkan-demos
//
//  Created by Rafael Sabino on 5/30/20.
//  Copyright © 2020 Rafael Sabino. All rights reserved.
//

//**************
//this code is heavily based off of sasha willems example tutorials.
//**************

#pragma once

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/cimport.h>

#include "EASTL/vector.h"
#include "texture_2d.h"
#include "node.h"
#include "texture_registry.h"
#include "vulkan/vulkan.h"

#include "obj_shape.h"
#include "core/device.h"
#include "mesh.h"
#include "assimp/texture.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <filesystem>

namespace vk
{

    enum class vertex_componets {
        VERTEX_COMPONENT_POSITION = 0x0,
        VERTEX_COMPONENT_NORMAL = 0x1,
        VERTEX_COMPONENT_COLOR = 0x2,
        VERTEX_COMPONENT_UV = 0x3,
        VERTEX_COMPONENT_TANGENT = 0x4,
        VERTEX_COMPONENT_BITANGENT = 0x5,
        VERTEX_COMPONENT_DUMMY_FLOAT = 0x6,
        VERTEX_COMPONENT_DUMMY_VEC4 = 0x7,
        VERTEX_COMPONENT_ALPHA = 0x8
    };

    
    using vertex_components = eastl::fixed_vector<vertex_componets,20, true>;
    /** @brief Stores vertex layout components for model loading and Vulkan vertex input and atribute bindings  */
    struct vertex_layout {
    public:
        
        vertex_layout(){}
        /** @brief Components used to generate vertices from */
        vertex_components components;
        
        vertex_layout( vertex_components &components)
        {
            this->components = components;
        }

        uint32_t stride()
        {
            uint32_t res = 0;
            for (auto& component : components)
            {
                switch (component)
                {
                case vertex_componets::VERTEX_COMPONENT_UV:
                    res += 2 * sizeof(float);
                    break;
                case vertex_componets::VERTEX_COMPONENT_DUMMY_FLOAT:
                    res += sizeof(float);
                    break;
                case vertex_componets::VERTEX_COMPONENT_DUMMY_VEC4:
                    res += 4 * sizeof(float);
                    break;
                default:
                    // All components except the ones listed above are made up of 3 floats
                    res += 3 * sizeof(float);
                }
            }
            return res;
        }
    };

    /** @brief Used to parametrize model loading */
    struct model_create_info {
        glm::vec3 center;
        glm::vec3 scale;
        glm::vec2 uvscale;
        VkMemoryPropertyFlags memoryPropertyFlags = 0;

        model_create_info() : center(glm::vec3(0.0f)), scale(glm::vec3(1.0f)), uvscale(glm::vec2(1.0f)) {};

        model_create_info(glm::vec3 scale, glm::vec2 uvscale, glm::vec3 center)
        {
            this->center = center;
            this->scale = scale;
            this->uvscale = uvscale;
        }

        model_create_info(float scale, float uvscale, float center)
        {
            this->center = glm::vec3(center);
            this->scale = glm::vec3(scale);
            this->uvscale = glm::vec2(uvscale);
        }

    };

    class assimp_mesh : public mesh
    {
        
    private:
        /** @brief Stores vertex and index base and counts for each part of a model */
        struct model_part {
            uint32_t vertex_base = 0;
            uint32_t vertex_count = 0;
            uint32_t index_base = 0;
            uint32_t index_count = 0;
        };
        
        struct dimension
        {
            glm::vec3 min = glm::vec3(FLT_MAX);
            glm::vec3 max = glm::vec3(-FLT_MAX);
            glm::vec3 size;
        } dim;
        
        //eastl::vector<model_part> _parts;
        
        uint32_t _vertex_size = 0;
        uint32_t _index_size = 0;
        
    public:
        
        assimp_mesh(){}
        assimp_mesh(device* dev):mesh(dev)
        {}
        
        
        inline void set_device(device* dev)
        {
            _device = dev;
        }
        
        virtual void draw_indexed(VkCommandBuffer command_buffer, uint32_t instance_count) override
        {
            EA_ASSERT(_index_size != 0);
            vkCmdDrawIndexed(command_buffer,_index_size, instance_count, 0, 0, 0);
        }
        virtual void draw(VkCommandBuffer command_buffer) override
        {
            EA_ASSERT(_vertex_size != 0);
            vkCmdDraw(command_buffer, _vertex_size, 1, 0,0 );
        }
        
        void create( VkMemoryPropertyFlags memoryPropertyFlags, eastl::vector<float>& vertexBuffer, eastl::vector<uint32_t>& indexBuffer)
        {
            _vertex_size = static_cast<uint32_t>(vertexBuffer.size());
            _index_size = static_cast<uint32_t>(indexBuffer.size());

            create_and_upload_buffer_void( _device->_graphics_command_pool, vertexBuffer,
                                     VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | memoryPropertyFlags,
                                     _vertex_buffer, _vertex_buffer_device_memory);

            create_and_upload_buffer_void(_device->_graphics_command_pool, indexBuffer,
                                     VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | memoryPropertyFlags,
                                     _index_buffer, _index_buffer_device_memory);
        }

        /** @brief Release all Vulkan resources of this model */
        virtual void destroy() override
        {
            mesh::destroy();
            vkDestroyBuffer(_device->_logical_device, _vertex_buffer, nullptr);
            vkFreeMemory(_device->_logical_device, _vertex_buffer_device_memory, nullptr);
            if (_index_buffer != VK_NULL_HANDLE)
            {
                vkDestroyBuffer(_device->_logical_device, _index_buffer, nullptr);
                vkFreeMemory(_device->_logical_device, _index_buffer_device_memory, nullptr);
            }
        }
    };

    class assimp_obj : public obj_shape
    {
    protected:
        uint32_t indexCount = 0;
        uint32_t vertexCount = 0;
        vk::device* _device = nullptr;
        
        vertex_layout _vertex_layout;
        model_create_info create_info = model_create_info(1.0f, 1.0f, 0.0f);

        static const uint32_t defaultFlags = aiProcess_ConvertToLeftHanded;
        using texture_path_vec = eastl::array< texture_path, 20>;
        using texture_type_vec = eastl::array<texture_path_vec, 20>;
        using mesh_textures = eastl::array<texture_type_vec, aiTextureType_UNKNOWN + 1>;
        
        mesh_textures _textures = {};
        
    private:
        
        void create( const aiScene* pScene, model_create_info *createInfo, vertex_layout& layout)
        {
            EA_ASSERT(pScene);
            
            //_parts.clear();
            //_parts.resize(pScene->mNumMeshes);
            
            glm::vec3 scale(1.0f);
            glm::vec2 uvscale(1.0f);
            glm::vec3 center(0.0f);
            if (createInfo)
            {
                scale = createInfo->scale;
                uvscale = createInfo->uvscale;
                center = createInfo->center;
            }

            eastl::vector<float> vertexBuffer;
            eastl::vector<uint32_t> indexBuffer;

            uint32_t indexCount = 0;
            uint32_t vertexCount = 0;
            
            //EA_ASSERT_MSG(pScene->mNumMeshes == 1, "Please merge all meshes into one");
            // Load meshes
            for (unsigned int i = 0; i < pScene->mNumMeshes; i++)
            {
                const aiMesh* paiMesh = pScene->mMeshes[i];

                //_parts[i] = {};
                //_parts[i].vertex_base = vertexCount;
                //_parts[i].index_base = indexCount;

                vertexCount += pScene->mMeshes[i]->mNumVertices;

                aiColor3D pColor(0.f, 0.f, 0.f);
                pScene->mMaterials[paiMesh->mMaterialIndex]->Get(AI_MATKEY_COLOR_DIFFUSE, pColor);
                
                const aiMaterial* material = pScene->mMaterials[paiMesh->mMaterialIndex];
                aiString path;
                
                for( int t = 0; t < (int)aiTextureType_UNKNOWN; ++t)
                {
                    for( int c  = 0; c < material->GetTextureCount((aiTextureType)t); ++c)
                    {
                        if(material->GetTexture( (aiTextureType)t, c,&path) == AI_SUCCESS)
                        {
                            texture_path new_texture = path.C_Str();
                            if(_textures[i][t][c] !=  new_texture )
                            {
                                EA_ASSERT_FORMATTED(_textures[i][t][c].empty(),
                                                    ("You have multiple textures of type %i on this mesh, (%s and %s)", t, _textures[i][t][c].c_str(), path.C_Str()));
                                _textures[i][t][c] = path.C_Str();
                            }
                        }
                    }
                }

                const aiVector3D Zero3D(0.0f, 0.0f, 0.0f);

                for (unsigned int j = 0; j < paiMesh->mNumVertices; j++)
                {
                    const aiVector3D* pPos = &(paiMesh->mVertices[j]);
                    const aiVector3D* pNormal = &(paiMesh->mNormals[j]);
                    const aiVector3D* pTexCoord = (paiMesh->HasTextureCoords(0)) ? &(paiMesh->mTextureCoords[0][j]) : &Zero3D;
                    const aiVector3D* pTangent = (paiMesh->HasTangentsAndBitangents()) ? &(paiMesh->mTangents[j]) : &Zero3D;
                    const aiVector3D* pBiTangent = (paiMesh->HasTangentsAndBitangents()) ? &(paiMesh->mBitangents[j]) : &Zero3D;

                    for (auto& component : layout.components)
                    {
                        switch (component) {
                        case vertex_componets::VERTEX_COMPONENT_POSITION:
                            vertexBuffer.push_back(pPos->x * scale.x + center.x);
                            vertexBuffer.push_back(pPos->y * scale.y + center.y);
                            vertexBuffer.push_back(pPos->z * scale.z + center.z);
                            break;
                        case vertex_componets::VERTEX_COMPONENT_NORMAL:
                            vertexBuffer.push_back(pNormal->x);
                            vertexBuffer.push_back(pNormal->y);
                            vertexBuffer.push_back(pNormal->z);
                            break;
                        case vertex_componets::VERTEX_COMPONENT_UV:
                            vertexBuffer.push_back(pTexCoord->x * uvscale.s);
                            vertexBuffer.push_back(pTexCoord->y * uvscale.t);
                            break;
                        case vertex_componets::VERTEX_COMPONENT_COLOR:
                            vertexBuffer.push_back(pColor.r);
                            vertexBuffer.push_back(pColor.g);
                            vertexBuffer.push_back(pColor.b);
                            vertexBuffer.push_back(1.0f);
                            break;
                        case vertex_componets::VERTEX_COMPONENT_TANGENT:
                            vertexBuffer.push_back(pTangent->x);
                            vertexBuffer.push_back(pTangent->y);
                            vertexBuffer.push_back(pTangent->z);
                            break;
                        case vertex_componets::VERTEX_COMPONENT_BITANGENT:
                            vertexBuffer.push_back(pBiTangent->x);
                            vertexBuffer.push_back(pBiTangent->y);
                            vertexBuffer.push_back(pBiTangent->z);
                            break;
                        // Dummy components for padding
                        case vertex_componets::VERTEX_COMPONENT_DUMMY_FLOAT:
                            vertexBuffer.push_back(0.0f);
                            break;
                        case vertex_componets::VERTEX_COMPONENT_DUMMY_VEC4:
                            vertexBuffer.push_back(0.0f);
                            vertexBuffer.push_back(0.0f);
                            vertexBuffer.push_back(0.0f);
                            vertexBuffer.push_back(0.0f);
                            break;
                        case vertex_componets::VERTEX_COMPONENT_ALPHA:
                            EA_FAIL_MSG("DON'T KNOW HOW TO HANDLE ALPHA YET");
                            break;
                        };
                    }

//                    dim.max.x = fmax(pPos->x, dim.max.x);
//                    dim.max.y = fmax(pPos->y, dim.max.y);
//                    dim.max.z = fmax(pPos->z, dim.max.z);
//
//                    dim.min.x = fmin(pPos->x, dim.min.x);
//                    dim.min.y = fmin(pPos->y, dim.min.y);
//                    dim.min.z = fmin(pPos->z, dim.min.z);
                }

//                dim.size = dim.max - dim.min;

                //_parts[i].vertex_count = paiMesh->mNumVertices;

                uint32_t indexBase = static_cast<uint32_t>(indexBuffer.size());
                for (unsigned int j = 0; j < paiMesh->mNumFaces; j++)
                {
                    const aiFace& Face = paiMesh->mFaces[j];
                    EA_ASSERT_MSG(Face.mNumIndices == 3, "This mesh needs to be triangulated");
                    indexBuffer.push_back(indexBase + Face.mIndices[0]);
                    indexBuffer.push_back(indexBase + Face.mIndices[1]);
                    indexBuffer.push_back(indexBase + Face.mIndices[2]);
                    indexCount += 3;
                }
            }
            
            vk::assimp_mesh* assimp_m = new assimp_mesh();
            assimp_m->set_device(_device);
            _meshes.push_back( assimp_m );
            assimp_m->create( createInfo->memoryPropertyFlags, vertexBuffer, indexBuffer );

        }
        
        bool load(const char* path)
        {
            bool result = true;
            Assimp::Importer Importer;
            const aiScene* pScene  = nullptr;

            // Load file
    #if defined(__ANDROID__)
            // Meshes are stored inside the apk on Android (compressed)
            // So they need to be loaded via the asset manager

            AAsset* asset = AAssetManager_open(androidApp->activity->assetManager, _path, AASSET_MODE_STREAMING);
            if (!asset) {
                LOGE("Could not load mesh from \"%s\"!",_path);
                return false;
            }
            assert(asset);
            size_t size = AAsset_getLength(asset);

            assert(size > 0);

            void *meshData = malloc(size);
            AAsset_read(asset, meshData, size);
            AAsset_close(asset);

            pScene = Importer.ReadFileFromMemory(meshData, size, defaultFlags);

            free(meshData);
    #else
            texture_path full_path = resource::resource_root + obj_shape::_shape_resource_path + _path;
            pScene = Importer.ReadFile(full_path.c_str(), defaultFlags);
            if (!pScene) {
                const char* error = Importer.GetErrorString();
                EA_FAIL_MSG(error);
                result = false;
            }
    #endif
            
            if (pScene)
            {
                create(pScene,&create_info, _vertex_layout);
            }
            
            return result;
        }
        
        
        void setup_vertex_layout()
        {
            _vertex_layout.components.push_back(vk::vertex_componets::VERTEX_COMPONENT_POSITION);
            _vertex_layout.components.push_back(vk::vertex_componets::VERTEX_COMPONENT_COLOR);
            _vertex_layout.components.push_back(vk::vertex_componets::VERTEX_COMPONENT_UV);
            _vertex_layout.components.push_back(vk::vertex_componets::VERTEX_COMPONENT_NORMAL);
            _vertex_layout.components.push_back(vk::vertex_componets::VERTEX_COMPONENT_TANGENT);
            _vertex_layout.components.push_back(vk::vertex_componets::VERTEX_COMPONENT_BITANGENT);
        }
        
    public:
        
        assimp_obj(device* dev,  const char* path):
            obj_shape(dev, path),_device(dev)
        {
            setup_vertex_layout();
        }
        
        assimp_obj()
        {
            setup_vertex_layout();
        }
        assimp_obj(device* dev)
        {
            setup_vertex_layout();
        }
        
        void set_vertex_layout(vk::vertex_components& comps)
        {
            _vertex_layout.components.clear();
            _vertex_layout.components = comps;
        }
        
        
        void set_device(device* dev)
        {
            _device = dev;
        }
        
        
        void set_path(const char* path)
        {
            _path = path;
        }
        
        void set_texture_relative_path(const char*  p, uint32_t id)
        {
            _textures[0][id][0] = p;
        }
        
        virtual texture_path get_texture(uint32_t id)  override
        {
            texture_path path;
            if(!_textures[0][id][0].empty())
            {
                std::filesystem::path base_path = _path.c_str();
                base_path = base_path.parent_path();
                
                path.sprintf("..%s%s/%s",obj_shape::_shape_resource_path.c_str(), base_path.c_str(), _textures[0][id][0].c_str());
                //path = _textures[0][id][0].c_str();

            }
            return path;
        }
        
        virtual void create() override
        {
            EA_ASSERT_MSG(!_path.empty(), "path to mesh was not proviced");
            bool result = load(_path.c_str());
            EA_ASSERT_FORMATTED(result, ("could not load mesh %s", _path.c_str()));
            //_meshes.push_back(&_mesh);
        }
        
        virtual void destroy() override
        {
            for( vk::mesh* m : _meshes)
            {
                m->destroy();
                delete m;
            }
            _meshes.clear();
        }
    };
}
