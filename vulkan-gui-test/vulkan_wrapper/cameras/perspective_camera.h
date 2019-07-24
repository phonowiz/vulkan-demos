//
//  perspective_camera.hpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 7/21/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#pragma once

#include "camera.h"


namespace vk
{
    class perspective_camera : public camera
    {
    public:
        perspective_camera(float fov = 0.7, float aspect = 1.0, float near = 0.1, float far = 500);
        
    private:
        
    };
}
