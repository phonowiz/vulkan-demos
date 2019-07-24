//
//  perspective_camera.cpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 7/21/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#include "perspective_camera.h"


using namespace vk;

perspective_camera::perspective_camera(float fov, float aspect, float near, float far)
: camera(fov, aspect, near, far)
{
    _projection_matrix =glm::perspective(fov, aspect, near, far);
    _focal_length = 1.0f/tanf(fov*.5f);
}

