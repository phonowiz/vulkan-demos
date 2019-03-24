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
        display_plane(device* device)
        {
            _device = device;
        }
        
        using mesh::allocate_gpu_memory;
        using mesh::destroy;
        void create();
        
    private:
        //todo: a lot of the mesh functions don't apply to this class, we'll need to make them private
    };
}
