//
//  transform.cpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 7/28/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#include "transform.h"



using namespace vk;

transform::transform() {
    update_transform_matrix();
}

void transform::update_transform_matrix() {
    mat_transform = glm::translate(position) * glm::mat4_cast(glm::quat(rotation)) * glm::scale(scale);
    transform_is_valid = false;
}

const glm::mat4 & transform::get_transform_matrix()
{
    if (transform_is_valid) { update_transform_matrix(); }
    return mat_transform;
}

glm::vec3 transform::forward() { return glm::quat(rotation) * glm::vec3(0, 0, 1); }
glm::vec3 transform::up() { return glm::quat(rotation) * glm::vec3(0, 1, 0); }
glm::vec3 transform::right() { return glm::quat(rotation) * glm::vec3(-1, 0, 0); }

std::ostream & vk::operator<<(std::ostream & os, const transform & t) {
    os << "- - - transform - - -" << std::endl;
    os << "position: " << t.position.x << ", " << t.position.y << ", " << t.position.z << std::endl;
    os << "rotation: " << t.rotation.x << ", " << t.rotation.y << ", " << t.rotation.z << std::endl;
    os << "scale: " << t.scale.x << ", " << t.scale.y << ", " << t.scale.z << std::endl;
    os << "matrix: " << std::endl;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            os << t.mat_transform[i][j] << " ";
        }
        os << std::endl;
    }
    os << "- - -         - - -" << std::endl;
    return os;
}

