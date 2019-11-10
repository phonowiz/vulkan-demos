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


static std::unordered_map<const char*,  shader_shared_ptr> shader_database;
static std::unordered_map<const char*,  mat_shared_ptr > material_database;

material_store::material_store()
{}

void material_store::create(device* device)
{
    _device = device;
    assert(_device != nullptr && "call setDevice() on the store object");
    
    shader_shared_ptr standard_vert = add_shader( "graphics/triangle.vert", shader::shader_type::VERTEX );
    shader_shared_ptr standard_frag = add_shader( "graphics/triangle.frag", shader::shader_type::FRAGMENT);
    
    shader_shared_ptr display_vert = add_shader("graphics/display_plane.vert", shader::shader_type::VERTEX);
    shader_shared_ptr display_farg = add_shader("graphics/display_plane.frag", shader::shader_type::FRAGMENT);
    
    shader_shared_ptr mrt_vert = add_shader("graphics/mrt.vert", shader::shader_type::VERTEX);
    shader_shared_ptr mrt_frag = add_shader("graphics/mrt.frag", shader::shader_type::FRAGMENT);
    
    shader_shared_ptr deferred_output_vert = add_shader("graphics/deferred_output.vert", shader::shader_type::VERTEX);
    shader_shared_ptr deferred_output_frag = add_shader("graphics/deferred_output.frag", shader::shader_type::FRAGMENT);
    
    shader_shared_ptr display_3d_texture_vert = add_shader("graphics/display_3d_texture.vert", shader::shader_type::VERTEX);
    shader_shared_ptr display_3d_texture_frag = add_shader("graphics/display_3d_texture.frag", shader::shader_type::FRAGMENT);
    
    shader_shared_ptr voxel_shader_vert = add_shader("graphics/voxelize.vert", shader::shader_type::VERTEX);
    shader_shared_ptr voxel_shader_frag = add_shader("graphics/voxelize.frag", shader::shader_type::FRAGMENT);
    
    shader_shared_ptr clear_3d_texture_comp =  add_shader("./compute/clear_3d_texture.comp", shader::shader_type::COMPUTE);
    shader_shared_ptr avg_texture_comp = add_shader("./compute/downsize.comp", shader::shader_type::COMPUTE);
    
    
    mat_shared_ptr standard_mat = CREATE_MAT<visual_material>("standard", standard_vert, standard_frag, device);
    add_material(standard_mat);
    
    mat_shared_ptr display_mat = CREATE_MAT<visual_material>("display", display_vert, display_farg, device);
    add_material(display_mat);
    
    mat_shared_ptr mrt_mat = CREATE_MAT<visual_material>("mrt", mrt_vert, mrt_frag, device);
    add_material(mrt_mat);
    
    mat_shared_ptr display_3d_texture_mat = CREATE_MAT<visual_material>("display_3d_texture",
                                                                    display_3d_texture_vert, display_3d_texture_frag, device);
    add_material(display_3d_texture_mat);
    
    mat_shared_ptr deferred_output_mat = CREATE_MAT<visual_material>("deferred_output",
                                                                   deferred_output_vert, deferred_output_frag, device);
    add_material(deferred_output_mat);
    
    mat_shared_ptr voxelizer_mat = CREATE_MAT<visual_material>("voxelizer", voxel_shader_vert, voxel_shader_frag, device);
    add_material(voxelizer_mat);
    
    mat_shared_ptr clear_3d_texture = CREATE_MAT<compute_material>("clear_3d_texture", clear_3d_texture_comp, device);
    add_material(clear_3d_texture);

    mat_shared_ptr downsize = CREATE_MAT<compute_material>("downsize", avg_texture_comp, device);
    add_material(downsize);
}

void material_store::add_material( mat_shared_ptr material)
{
    material_database[material->_name] = material;
}


shader_shared_ptr material_store::add_shader(const char *shaderPath, shader::shader_type shaderType)
{
    
    shader_shared_ptr result = nullptr;
    if(shader_database.count(shaderPath) == 0)
    {
        result = std::make_shared<shader>(_device, shaderPath, shaderType);
        shader_database[shaderPath] = result;
    }
    else
    {
        result = shader_database[shaderPath];
    }
    
    return result;
}

mat_shared_ptr material_store::get_material(const char* name) const
{
    assert(material_database.count(name) != 0);
    return material_database[name];
}

shader_shared_ptr const   material_store::find_shader_using_path(const char* path)const
{
    assert(shader_database.count(path) != 0);
    return shader_database[path];
}

void material_store::destroy()
{
    for (std::pair<const char* , shader_shared_ptr> pair : shader_database)
    {
        pair.second->destroy();
    }
    
    for (std::pair<const char* , mat_shared_ptr> pair : material_database)
    {
        pair.second->destroy();
    }
}
material_store::~material_store()
{
}
