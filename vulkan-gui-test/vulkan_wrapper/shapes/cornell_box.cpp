//
//  cornell_box.cpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 8/9/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#include "cornell_box.h"


using namespace vk;

cornell_box::cornell_box( vk::device* device):
obj_shape(device, "cornell_box.obj")
{    
    _wall_colors[0] = glm::vec3(0.0f, 1.0f, 0.0f);
    _wall_colors[1] = glm::vec3(1.0f); //bottom
    _wall_colors[2] = glm::vec3(1.0f); //top
    _wall_colors[3] = glm::vec3(1.0f, 0.0f, 0.0f);
    _wall_colors[4] = glm::vec3(1.0f); //back
    _wall_colors[5] = glm::vec3(1.0f);
    _wall_colors[6] = glm::vec3(1.0f);
}


void cornell_box::set_diffuse( glm::vec3 diffuse)
{
    for(size_t i = 0 ; i < _wall_colors.size(); ++i)
    {
        _wall_colors[i] = diffuse;
    }
}
void cornell_box::create()
{
    tinyobj::attrib_t vertex_attributes;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    
    std::string error_string;
    std::string warn_string;
    
    std::string  full_path = resource::resource_root + obj_shape::_shape_resource_path + _path;
    
    bool success = tinyobj::LoadObj(&vertex_attributes, &shapes, &materials, &warn_string, &error_string, full_path.c_str());
    
    assert(success && "check errorString variable");

    
    int i = 0;
    for(tinyobj::shape_t shape:  shapes)
    {
        tinyobj::material_t mat {};
        mat.diffuse[0] = _wall_colors[i].r;
        mat.diffuse[1] = _wall_colors[i].g;
        mat.diffuse[2] = _wall_colors[i].b;        
        
        if( i == 5 || i == 6)
        {
            //we skip these because these are random two boxes embedded within the cornell box
            continue;
        }
        mesh* m = new mesh(_device, vertex_attributes, shape, mat);
        _meshes.push_back(m);
        ++i;
    }
}
