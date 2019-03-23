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
static std::unordered_map<const char*,  MaterialSharedPtr > materialDatabase;

material_store::material_store()
{}

void material_store::createStore(device* device)
{
    _device = device;
    assert(_device != nullptr && "call setDevice() on the store object");
    
    ShaderSharedPtr standardVert = AddShader( "triangle.vert", shader::ShaderType::VERTEX );
    ShaderSharedPtr standardFrag = AddShader( "triangle.frag", shader::ShaderType::FRAGMENT);
    
    ShaderSharedPtr displayVert = AddShader("display_plane.vert", shader::ShaderType::VERTEX);
    ShaderSharedPtr displayFrag = AddShader("display_plane.frag", shader::ShaderType::FRAGMENT);
    
    MaterialSharedPtr standardMat = CREATE_MAT<material>("standard", standardVert, standardFrag, device);
    AddMaterial(standardMat);
    
    MaterialSharedPtr displayMat = CREATE_MAT<material>("display", displayVert, displayFrag, device);
    AddMaterial(displayMat);
}

void material_store::AddMaterial( MaterialSharedPtr material)
{
    materialDatabase[material->_name] = material;
}


ShaderSharedPtr material_store::AddShader(const char *shaderPath, shader::ShaderType shaderType)
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

MaterialSharedPtr material_store::getMaterial(const char* name) const
{
    assert(materialDatabase.count(name) != 0);
    return materialDatabase[name];
}

ShaderSharedPtr const   material_store::findShaderUsingPath(const char* path)const
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
    
    for (std::pair<const char* , MaterialSharedPtr> pair : materialDatabase)
    {
        pair.second->destroy();
    }
}
material_store::~material_store()
{
}
