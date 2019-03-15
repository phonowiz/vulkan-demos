//
//  plane.hpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 3/12/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#pragma once

#include "mesh.h"
#include "physical_device.h"

namespace vk
{
    class plane : private Mesh
    {
    public:
        plane(PhysicalDevice* device)
        {
            _device = device;
        }
        
        using Mesh::allocateGPUMemory;
        using Mesh::destroy;
        void create();
    };
}
