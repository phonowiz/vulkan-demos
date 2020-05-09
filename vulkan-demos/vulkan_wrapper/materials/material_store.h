//
//  MaterialStore.hpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 2/12/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#pragma once

#include "EASTL/shared_ptr.h"
#include "visual_material.h"
#include "compute_material.h"

#include "shader.h"

namespace vk
{
    class device;
    
    class material_store : public object
    {
        
    public:
        material_store & operator=(const material_store&) = delete;
        material_store(const material_store&) = delete;
        material_store & operator=(material_store&) = delete;
        material_store(material_store&) = delete;
        
        ~material_store();
        template <typename T>
        inline eastl::shared_ptr<T> GET_MAT(const char* material_name)
        {
            auto t = get_material(material_name);
            return eastl::static_pointer_cast<T>(t);
        }
        
        material_store();
        
        void create(device* device);
        virtual void destroy() override;
    private:

        template <typename T, typename ...ARGS>
        inline static mat_shared_ptr CREATE_MAT( ARGS... args)
        {
            eastl::shared_ptr<T> pointer = eastl::make_shared<T> (args...);
            return eastl::static_shared_pointer_cast<material_base>(pointer);
        }
        
        mat_shared_ptr get_material(const char* name);
        
        inline shader_shared_ptr const  find_shader_using_path(const char* path)const ;
        shader_shared_ptr add_shader(const char* shaderPath, shader::shader_type shaderType);
        void add_material( eastl::shared_ptr<material_base> material);
        
        device* _device = nullptr;
        
        
    };
}

