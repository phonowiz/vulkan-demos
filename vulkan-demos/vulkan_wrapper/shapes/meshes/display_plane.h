//
//  plane.hpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 3/12/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#pragma once

#include "mesh.h"
#include "device.h"

namespace vk
{
    class display_plane : public mesh
    {
    public:
        
        display_plane(){}
        
        void set_device(vk::device* dev)
        {
            _device = dev;
        }
        display_plane(device* device)
        {
            _device = device;
        }
        
        void create();
        
    private:
    };
}
