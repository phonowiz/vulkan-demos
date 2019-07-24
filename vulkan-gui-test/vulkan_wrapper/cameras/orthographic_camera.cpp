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
}

