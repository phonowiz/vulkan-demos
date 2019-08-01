//
//  plane.hpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 3/12/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#pragma once

#include "shapes/mesh.h"
#include "device.h"

namespace vk
{
    class display_plane : public mesh
    {
    public:
        display_plane(device* device)
        {
            _device = device;
            create();
        }
        
        void create();
        
    private:
    };
}
