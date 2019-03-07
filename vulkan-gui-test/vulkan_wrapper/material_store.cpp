//
//  MaterialStore.cpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 2/12/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#include "material_store.h"
#include "physical_device.h"
#include <unordered_map>


using namespace vk;


static std::unordered_map<const char*,  ShaderSharedPtr> shaderDatabase;
static std::unordered_map<const char*,  MaterialSharedPtr > materialDatabase;

MaterialStore::MaterialStore()
{}

void MaterialStore::createStore(PhysicalDevice* device)
{
    _device = device;
    assert(_device != nullptr && "call setDevice() on the store object");
    
    ShaderSharedPtr standardVert = AddShader( "triangle.vert", Shader::ShaderType::VERTEX );
    ShaderSharedPtr standardFrag = AddShader( "triangle.frag", Shader::ShaderType::FRAGMENT);
    
    MaterialSharedPtr standardMat = CREATE_MAT<Material>("standard", standardVert, standardFrag, device);
    AddMaterial(standardMat);
}

void MaterialStore::AddMaterial( MaterialSharedPtr material)
{
    materialDatabase[material->_name] = material;
}


ShaderSharedPtr MaterialStore::AddShader(const char *shaderPath, Shader::ShaderType shaderType)
{
    
    ShaderSharedPtr result = nullptr;
    if(shaderDatabase.count(shaderPath) == 0)
    {
        result = std::make_shared<Shader>(_device, shaderPath, shaderType);
        shaderDatabase[shaderPath] = result;
    }
    else
    {
        result = shaderDatabase[shaderPath];
    }
    
    return result;
}

MaterialSharedPtr MaterialStore::getMaterial(const char* name) const
{
    assert(materialDatabase.count(name) != 0);
    return materialDatabase[name];
}

ShaderSharedPtr const   MaterialStore::findShaderUsingPath(const char* path)const
{
    assert(shaderDatabase.count(path) != 0);
    return shaderDatabase[path];
}

MaterialStore const & MaterialStore::getInstance()
{
    static MaterialStore instance;
    return instance;
}

void MaterialStore::destroy()
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
MaterialStore::~MaterialStore()
{
}
