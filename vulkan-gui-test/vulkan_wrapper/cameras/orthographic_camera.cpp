//
//  orthographic_camera.cpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 7/21/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#include "orthographic_camera.h"


using namespace vk;

orthographic_camera::orthographic_camera() :
camera(glm::ortho(-1, 1, -1, 1, -1, 1))
{
    _near = -1.0f;
    _far = 1.0f;
    _aspect = 1.0f;
}

orthographic_camera::orthographic_camera(float view_space_width, float view_space_height, float view_space_depth):
camera(glm::mat4(1.0f))
{
    _aspect = view_space_width/view_space_height;
    _left = view_space_width * -.5f;
    _right = -_left;
    _top = view_space_height * .5f;
    _bottom = -_top;
    //anything behind the camera will not be projected
    _near = 0.0f;
    _far = view_space_depth ;
    _focal_length = _near;
    
    _projection_matrix = glm::ortho(_left, _right, _bottom, _top, _near, _far);
    
    /*
     GLM was originally designed for OpenGL, where the Y coordinate of the clip coordinates is inverted.
     The easiest way to compensate for that is to flip the sign on the scaling factor of the Y axis in the projection matrix.
     If you don't do this, then the image will be rendered upside down.
     */
    
    _projection_matrix[1][1] *= -1.0f;
}

