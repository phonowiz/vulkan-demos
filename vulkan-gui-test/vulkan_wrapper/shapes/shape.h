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
#include "../object.h"
#include "../graphics_pipeline.h"


namespace vk {
    
    class obj_shape  : object
    {
    protected:
        obj_shape(vk::device* device){ _device = device; };
    public:
        obj_shape(device* device, const char* path);
        
        virtual void destroy() override;
        
        void draw(VkCommandBuffer commnad_buffer, vk::graphics_pipeline& pipeline);
        
        virtual void create();
        static const std::string _shape_resource_path;
        
    protected:
        
        std::vector<mesh*> _meshes;
        device* _device = nullptr;
        const char* _path = nullptr;
    };
}
