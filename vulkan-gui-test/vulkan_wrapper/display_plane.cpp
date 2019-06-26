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
    //std::vector<Vertex>& vertices;
    // Vertex(glm::vec3 pos, glm::vec3 color, glm::vec2 uvCoord, glm::vec3 normal)
    _vertices =
    {
        vertex({-1.0f, -1.0f, 0.10f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}),
        vertex({1.f, 1.f, 0.10f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}),
        vertex({-1.f, 1.f, 0.10f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 0.0f}),
        vertex({1.0f, -1.f, 0.10f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}),

        vertex({-1.f, -1.f, .10f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}),
        vertex({1.f, 1.f, .10f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}),
        vertex({-1.f, 1.f,.10f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 0.0f}),
        vertex({1.f, -1.f,.10f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, 0.0f})
    };
    
    
    _indices =
    {
        //4,5,6,4,7,5,
        0,1,2,0,3,1,
        4,5,6,4,7,5
    };
    
    allocate_gpu_memory();
}
