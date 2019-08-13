//
//  plane.cpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 3/12/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#include "display_plane.h"

#include "vertex.h"
using namespace vk;


void display_plane::create()
{
    _vertices =
    {
        vertex({-1.0f, -1.0f, 0.10f}, {1.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}),
        vertex({1.f, 1.f, 0.10f}, {0.0f, 1.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}),
        vertex({-1.f, 1.f, 0.10f}, {0.0f, 0.0f, 1.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 0.0f}),
        vertex({1.0f, -1.f, 0.10f}, {0.0f, 1.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}),

        vertex({-1.f, -1.f, .10f}, {1.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}),
        vertex({1.f, 1.f, .10f}, {0.0f, 1.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}),
        vertex({-1.f, 1.f,.10f}, {0.0f, 0.0f, 1.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 0.0f}),
        vertex({1.f, -1.f,.10f}, {0.0f, 1.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, 0.0f})
    };
    
    
    _indices =
    {
        5,7,4,6,5,4,
        1,3,0,2,1,0
    };
    
    allocate_gpu_memory();
}
