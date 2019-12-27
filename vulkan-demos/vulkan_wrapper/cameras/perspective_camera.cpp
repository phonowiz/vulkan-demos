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
    
    /*
     GLM was originally designed for OpenGL, where the Y coordinate of the clip coordinates is inverted.
     The easiest way to compensate for that is to flip the sign on the scaling factor of the Y axis in the projection matrix.
     If you don't do this, then the image will be rendered upside down.
     */
    _projection_matrix[1][1] *= -1.0f;
    
    _focal_length = 1.0f/tanf(fov*.5f);
}

