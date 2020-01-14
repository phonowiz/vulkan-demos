//
//  shape.cpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 8/8/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#include "obj_shape.h"
#include "mesh.h"

//note: tinyobjloader implementation is included by mesh.cpp. Shape depends on mesh, therefore it is assumed that
//tinyobjloader implementation has already been included.  Including tinyobjloader implementation here will produce
//link errors.

//#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include <string>
#include <vector>
#include "../pipelines/graphics_pipeline.h"


using namespace vk;

const std::string obj_shape::_shape_resource_path =  "/models/";


obj_shape::obj_shape(device* device, const char* path)
{
    _device = device;
    _path = path;
}


void obj_shape::create()
{
    tinyobj::attrib_t vertex_attributes;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    
    std::string error_string;
    std::string warn_string;
    
    std::string  full_path = resource::resource_root + obj_shape::_shape_resource_path + _path;
    
    bool success = tinyobj::LoadObj(&vertex_attributes, &shapes, &materials, &warn_string, &error_string, full_path.c_str());
    
    assert(success && "check errorString variable");
    
    tinyobj::material_t mat {};
    mat.diffuse[0] = _diffuse.r;
    mat.diffuse[1] = _diffuse.g;
    mat.diffuse[2] = _diffuse.b;
    
    int i = 0;
    for(tinyobj::shape_t shape:  shapes)
    {        
        if(materials.size() != 0)
        {
            mat = materials[i];
        }

        mesh* m = new mesh(_device, vertex_attributes, shape, mat);
        _meshes.push_back(m);
        ++i;
    }
}

void obj_shape::set_diffuse(glm::vec3 diffuse)
{
    _diffuse = diffuse;
}
void obj_shape::destroy()
{
    for( mesh* m : _meshes)
    {
        delete m;
        m = nullptr;
    }
    _meshes.clear();
    
}
