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
    class device;
    
    class material_store : public object
    {
        using MaterialStoreSharedPtr = std::shared_ptr<material_store>;
        
    public:
        static material_store const& getInstance();
        
        ~material_store();
        template <typename T>
        inline static std::shared_ptr<T> GET_MAT(const char* materialName)
        {
            return std::static_pointer_cast<T>(material_store::getInstance().getMaterial(materialName));
        }
        
        material_store();
        void createStore(device* device);
        virtual void destroy() override;
        
    private:
        template <typename T, typename ...ARGS>
        inline static MaterialSharedPtr CREATE_MAT( ARGS... args)
        {
            std::shared_ptr<T> pointer = std::make_shared<T> (args...);
            return std::static_pointer_cast<material>(pointer);
        }
        
        MaterialSharedPtr getMaterial(const char* name) const;
        inline ShaderSharedPtr const  findShaderUsingPath(const char* path)const ;
        ShaderSharedPtr AddShader(const char* shaderPath, shader::ShaderType shaderType);
        void AddMaterial( std::shared_ptr<material> material);
        
        void setDevice(device* device){ _device = device; };

        void InitShaders();
        void InitMaterials();

        
        device* _device;
        
        
        material_store(material_store const &) = delete;
        void operator=(material_store const &) = delete;
        
        
    };
}

