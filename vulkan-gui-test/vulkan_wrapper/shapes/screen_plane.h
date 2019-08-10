//
//  screen_plane.hpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 8/9/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#pragma once

#include "obj_shape.h"
#include "../display_plane.h"
namespace vk
{
    class screen_plane : public obj_shape
    {
    public:
        
        inline screen_plane(vk::device* device):
        obj_shape(device),
        _display_plane( device ){  }
        
        inline void create()
        {
            _display_plane.create();
            
            _meshes.push_back(&_display_plane);
            _path = "display_plane";
            
        }
        
    private:
        
        display_plane _display_plane;
    };
}
