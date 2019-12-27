//
//  camera.cpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 7/21/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#include "camera.h"

using namespace vk;


const glm::mat4 & camera::get_projection_matrix() const
{
    return _projection_matrix;
}

void camera::set_projection_matrix(glm::mat4 projection_matrix)
{
    _projection_matrix = projection_matrix;
    _projection_matrix_has_changed = true;
    _dirty_projection_matrix = true;
}

void camera::update_view_matrix() {
    view_matrix = glm::lookAt(position, position + forward, up);
}

