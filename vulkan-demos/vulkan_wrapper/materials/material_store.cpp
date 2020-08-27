//
//  MaterialStore.cpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 2/12/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#include "material_store.h"
#include "device.h"
#include "EASTL/unordered_map.h"
#include "EASTL/fixed_string.h"
#include <fstream>
#include <iostream>

using namespace vk;

static eastl::unordered_map<eastl::string,  shader_shared_ptr> shader_database;
static eastl::unordered_map<eastl::string,  mat_shared_ptr > material_database;

material_store::material_store()
{}

void material_store::create(device* device)
{
    _device = device;
    EA_ASSERT_MSG(_device != nullptr, "call setDevice() on the store object");
    
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
    
    shader_shared_ptr vsm_vert = add_shader("graphics/vsm.vert", shader::shader_type::VERTEX);
    shader_shared_ptr vsm_frag = add_shader("graphics/vsm.frag", shader::shader_type::FRAGMENT);
    
    
    shader_shared_ptr voxel_shader_vert = add_shader("graphics/voxelize.vert", shader::shader_type::VERTEX);
    shader_shared_ptr voxel_shader_frag = add_shader("graphics/voxelize.frag", shader::shader_type::FRAGMENT);
    
    shader_shared_ptr clear_3d_texture_comp =  add_shader("compute/clear_3d_texture.comp", shader::shader_type::COMPUTE);
    shader_shared_ptr avg_texture_comp = add_shader("compute/downsize.comp", shader::shader_type::COMPUTE);
    
    shader_shared_ptr gauss_blur_vert = add_shader("graphics/gaussblur.vert", shader::shader_type::VERTEX);
    shader_shared_ptr gauss_blur_frag = add_shader("graphics/gaussblur.frag", shader::shader_type::FRAGMENT);
    
    
    shader_shared_ptr color_vert = add_shader("graphics/color.vert", shader::shader_type::VERTEX);
    shader_shared_ptr color_frag = add_shader("graphics/color.frag", shader::shader_type::FRAGMENT);
    
    mat_shared_ptr color_mat = CREATE_MAT<visual_material>("color", color_vert, color_frag, device);
    add_material(color_mat);

    shader_shared_ptr atmospheric_vert = add_shader("graphics/atmospheric.vert", shader::shader_type::VERTEX);
    shader_shared_ptr atmospheric_frag = add_shader("graphics/atmospheric.frag", shader::shader_type::FRAGMENT);
    
    mat_shared_ptr atmosheric_mat = CREATE_MAT<visual_material>("atmospheric", atmospheric_vert, atmospheric_frag, device);
    add_material(atmosheric_mat);

    shader_shared_ptr fxaa_vert = add_shader("graphics/fxaa.vert", shader::shader_type::VERTEX);
    shader_shared_ptr fxaa_frag = add_shader("graphics/fxaa.frag", shader::shader_type::FRAGMENT);
    
    mat_shared_ptr fxaa_mat = CREATE_MAT<visual_material>("fxaa", fxaa_vert, fxaa_frag, device);
    add_material(fxaa_mat);

    shader_shared_ptr luminance_vert = add_shader("graphics/luminance.vert", shader::shader_type::VERTEX);
    shader_shared_ptr luminance_frag = add_shader("graphics/luminance.frag", shader::shader_type::FRAGMENT);
    
    mat_shared_ptr luminance_mat = CREATE_MAT<visual_material>("luminance", luminance_vert, luminance_frag, device);
    add_material(luminance_mat);
    
    shader_shared_ptr ibl_vert = add_shader("graphics/ibl.vert", shader::shader_type::VERTEX);
    shader_shared_ptr ibl_frag = add_shader("graphics/ibl.frag", shader::shader_type::FRAGMENT);
    
    mat_shared_ptr ibl_mat = CREATE_MAT<visual_material>("ibl", ibl_vert, ibl_frag, device);
    add_material(ibl_mat);
    
    shader_shared_ptr radiance_map_vert = add_shader("graphics/radiance_map.vert", shader::shader_type::VERTEX);
    shader_shared_ptr radiance_map_frag = add_shader("graphics/radiance_map.frag", shader::shader_type::FRAGMENT);
    
    mat_shared_ptr radiance_map_mat = CREATE_MAT<visual_material>("radiance_map", radiance_map_vert, radiance_map_frag, device);
    add_material(radiance_map_mat);
    
    shader_shared_ptr pbr_vert = add_shader("graphics/pbr.vert", shader::shader_type::VERTEX);
    shader_shared_ptr pbr_frag = add_shader("graphics/pbr.frag", shader::shader_type::FRAGMENT);
    
    mat_shared_ptr pbr_mat = CREATE_MAT<visual_material>("pbr", pbr_vert, pbr_frag, device);
    add_material(pbr_mat);
    
    mat_shared_ptr gaussblur_mat = CREATE_MAT<visual_material>("gaussblur", gauss_blur_vert, gauss_blur_frag, device);
    add_material(gaussblur_mat);
    
    mat_shared_ptr vsm_mat = CREATE_MAT<visual_material>("vsm", vsm_vert, vsm_frag, device);
    add_material(vsm_mat);

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
    std::cout << "adding material " <<  material->_name << std::endl;
    
    eastl::string key = material->_name;
    material_database[key] = material;
}


shader_shared_ptr material_store::add_shader(const char *shaderPath, shader::shader_type shaderType)
{
    
    shader_shared_ptr result = nullptr;
    if(shader_database.count(shaderPath) == 0)
    {
        result = eastl::make_shared<shader>(_device, shaderPath, shaderType);
        eastl::string key = shaderPath;
        shader_database[key] = result;
    }
    else
    {
        result = shader_database[shaderPath];
    }
    
    return result;
}

mat_shared_ptr material_store::get_material(const char* name)
{
    EA_ASSERT_FORMATTED(material_database.count(name) != 0, ("material %s was not found, check spelling.", name));
    mat_shared_ptr tmp = material_database[name];
    if(tmp->get_in_use())
    {
       if( material_database[name]->get_instance_type()  == visual_material::get_material_type())
       {
           tmp = CREATE_MAT<visual_material>( eastl::static_shared_pointer_cast<visual_material>(tmp) );
       }
       else
       {
           tmp = CREATE_MAT<compute_material>( eastl::static_shared_pointer_cast<compute_material>(tmp) );
       }
    }
    tmp->set_in_use();
    return tmp;
}

shader_shared_ptr const   material_store::find_shader_using_path(const char* path)const
{
    EA_ASSERT_FORMATTED(shader_database.count(path) != 0, ("Shader not found on path: %s", path));
    return shader_database[path];
}

void material_store::destroy()
{
    for (eastl::pair<eastl::string , shader_shared_ptr> pair : shader_database)
    {
        pair.second->destroy();
    }
    
    for (eastl::pair<eastl::string , mat_shared_ptr> pair : material_database)
    {
        pair.second->destroy();
    }
}
material_store::~material_store()
{
}
