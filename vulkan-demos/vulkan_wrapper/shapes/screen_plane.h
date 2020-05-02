//
//  screen_plane.hpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 8/9/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#pragma once

#include "obj_shape.h"
#include "display_plane.h"
namespace vk
{
    class screen_plane : public obj_shape
    {
    public:
        
        inline screen_plane(){}
        
        inline screen_plane(vk::device* device):
        obj_shape(device),
        _display_plane( device ){  }
        
        inline void create() override
        {
            _meshes.push_back(&_display_plane);
            _path = "display_plane";
            _display_plane.create();
        }
        
        void set_device(vk::device* dev)
        {
            _display_plane.set_device(dev);
        }
        
        virtual void destroy() override
        {
            _display_plane.destroy();
            _meshes.clear();
            obj_shape::destroy();
        }
        
    private:
        
        display_plane _display_plane;
    };
}
