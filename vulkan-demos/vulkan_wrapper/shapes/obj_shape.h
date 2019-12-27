//
//  shape.hpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 8/8/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#pragma once


#include <string>
#include <vector>

#include "mesh.h"
#include "../core/object.h"
#include "../pipelines/graphics_pipeline.h"
#include "transform.h"


namespace vk {
    
    //note: the name obj_shape comes from the fact that these objects are created by reading .obj files
    class obj_shape  : object
    {
    protected:
        obj_shape(vk::device* device){ _device = device; };
    public:
        obj_shape(device* device, const char* path);
        
        virtual void destroy() override;
        
        void draw(VkCommandBuffer commnad_buffer, vk::graphics_pipeline& pipeline, uint32_t obj_id);
        
        virtual void create();
        static const std::string _shape_resource_path;
        
        virtual void set_diffuse(glm::vec3 diffuse);
        
        vk::transform transform;

    protected:
        
        glm::vec3 _diffuse = glm::vec3(1.0f);
        std::vector<mesh*> _meshes;
        device* _device = nullptr;
        const char* _path = nullptr;
    };
}
