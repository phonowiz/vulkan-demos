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


static std::unordered_map<const char*,  shader_shared_ptr> shaderDatabase;
static std::unordered_map<const char*,  material_shared_ptr > materialDatabase;

material_store::material_store()
{}

void material_store::create(device* device)
{
    _device = device;
    assert(_device != nullptr && "call setDevice() on the store object");
    
    shader_shared_ptr standard_vert = add_shader( "triangle.vert", shader::ShaderType::VERTEX );
    shader_shared_ptr standard_frag = add_shader( "triangle.frag", shader::ShaderType::FRAGMENT);
    
    shader_shared_ptr display_vert = add_shader("display_plane.vert", shader::ShaderType::VERTEX);
    shader_shared_ptr display_farg = add_shader("display_plane.frag", shader::ShaderType::FRAGMENT);
    
    shader_shared_ptr mrt_vert = add_shader("mrt.vert", shader::ShaderType::VERTEX);
    shader_shared_ptr mrt_frag = add_shader("mrt.frag", shader::ShaderType::FRAGMENT);
    
    shader_shared_ptr deferred_output_vert = add_shader("deferred_output.vert", shader::ShaderType::VERTEX);
    shader_shared_ptr deferred_output_frag = add_shader("deferred_output.frag", shader::ShaderType::FRAGMENT);
    
    
    
    material_shared_ptr standard_mat = CREATE_MAT<material>("standard", standard_vert, standard_frag, device);
    add_material(standard_mat);
    
    material_shared_ptr display_mat = CREATE_MAT<material>("display", display_vert, display_farg, device);
    add_material(display_mat);
    
    material_shared_ptr mrt_mat = CREATE_MAT<material>("mrt", mrt_vert, mrt_frag, device);
    add_material(mrt_mat);
    
    material_shared_ptr deferred_output_mat = CREATE_MAT<material>("deferred_output",
                                                                   deferred_output_vert, deferred_output_frag, device);
    add_material(deferred_output_mat);
    
}

void material_store::add_material( material_shared_ptr material)
{
    materialDatabase[material->_name] = material;
}


shader_shared_ptr material_store::add_shader(const char *shaderPath, shader::ShaderType shaderType)
{
    
    shader_shared_ptr result = nullptr;
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

shader_shared_ptr const   material_store::find_shader_using_path(const char* path)const
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
    for (std::pair<const char* , shader_shared_ptr> pair : shaderDatabase)
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
