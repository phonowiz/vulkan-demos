//
//  MaterialStore.hpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 2/12/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#pragma once

#include <vector>
#include <string>

#include "material.h"
#include "shader.h"

namespace vk
{
    class PhysicalDevice;
    
    class MaterialStore : public object
    {
        using MaterialStoreSharedPtr = std::shared_ptr<MaterialStore>;
        
    public:
        static MaterialStore const& getInstance();
        
        ~MaterialStore();
        template <typename T>
        inline static std::shared_ptr<T> GET_MAT(const char* materialName)
        {
            return std::static_pointer_cast<T>(MaterialStore::getInstance().getMaterial(materialName));
        }
        
        MaterialStore();
        void createStore(PhysicalDevice* device);
        virtual void destroy() override;
        
    private:
        template <typename T, typename ...ARGS>
        inline static MaterialSharedPtr CREATE_MAT( ARGS... args)
        {
            std::shared_ptr<T> pointer = std::make_shared<T> (args...);
            return std::static_pointer_cast<Material>(pointer);
        }
        
        MaterialSharedPtr getMaterial(const char* name) const;
        inline ShaderSharedPtr const  findShaderUsingPath(const char* path)const ;
        ShaderSharedPtr AddShader(const char* shaderPath, Shader::ShaderType shaderType);
        void AddMaterial( std::shared_ptr<Material> material);
        
        void setDevice(PhysicalDevice* device){ _device = device; };

        void InitShaders();
        void InitMaterials();

        
        PhysicalDevice* _device;
        
        
        MaterialStore(MaterialStore const &) = delete;
        void operator=(MaterialStore const &) = delete;
        
        
    };
}

