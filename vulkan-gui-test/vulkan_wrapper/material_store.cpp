//
//  MaterialStore.cpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 2/12/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#include "material_store.h"
#include "device.h"
#include <unordered_map>


using namespace vk;


static std::unordered_map<const char*,  ShaderSharedPtr> shaderDatabase;
static std::unordered_map<const char*,  material_shared_ptr > materialDatabase;

material_store::material_store()
{}

void material_store::createStore(device* device)
{
    _device = device;
    assert(_device != nullptr && "call setDevice() on the store object");
    
    ShaderSharedPtr standardVert = add_shader( "triangle.vert", shader::ShaderType::VERTEX );
    ShaderSharedPtr standardFrag = add_shader( "triangle.frag", shader::ShaderType::FRAGMENT);
    
    ShaderSharedPtr displayVert = add_shader("display_plane.vert", shader::ShaderType::VERTEX);
    ShaderSharedPtr displayFrag = add_shader("display_plane.frag", shader::ShaderType::FRAGMENT);
    
    material_shared_ptr standardMat = CREATE_MAT<material>("standard", standardVert, standardFrag, device);
    add_material(standardMat);
    
    material_shared_ptr displayMat = CREATE_MAT<material>("display", displayVert, displayFrag, device);
    add_material(displayMat);
}

void material_store::add_material( material_shared_ptr material)
{
    materialDatabase[material->_name] = material;
}


ShaderSharedPtr material_store::add_shader(const char *shaderPath, shader::ShaderType shaderType)
{
    
    ShaderSharedPtr result = nullptr;
    if(shaderDatabase.count(shaderPath) == 0)
    {
        result = std::make_shared<shader>(_device, shaderPath, shaderType);
        shaderDatabase[shaderPath] = result;
    }
    else
    {
        result = shaderDatabase[shaderPath];
    }
    
    return result;
}

material_shared_ptr material_store::get_material(const char* name) const
{
    assert(materialDatabase.count(name) != 0);
    return materialDatabase[name];
}

ShaderSharedPtr const   material_store::find_shader_using_path(const char* path)const
{
    assert(shaderDatabase.count(path) != 0);
    return shaderDatabase[path];
}

material_store const & material_store::getInstance()
{
    static material_store instance;
    return instance;
}

void material_store::destroy()
{
    for (std::pair<const char* , ShaderSharedPtr> pair : shaderDatabase)
    {
        pair.second->destroy();
    }
    
    for (std::pair<const char* , material_shared_ptr> pair : materialDatabase)
    {
        pair.second->destroy();
    }
}
material_store::~material_store()
{
}
