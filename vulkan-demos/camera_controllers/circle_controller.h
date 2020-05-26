//
//  circle_controller.hpp
//  vulkan-demos
//
//  Created by Rafael Sabino on 5/25/20.
//  Copyright © 2020 Rafael Sabino. All rights reserved.
//

#pragma once
//
//  first_person_controller.hpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 7/21/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#pragma once

#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif

#include "glm/gtx/rotate_vector.hpp"

#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>
#include "../vulkan_wrapper/cameras/camera.h"
#include "vulkan_wrapper/cameras/perspective_camera.h"

#include <cmath>

class circle_controller {
public:
    
    
    circle_controller(vk::camera * camera, float radius, glm::vec3 center, glm::vec3 lookat = glm::vec3(0.0f)):
    _rendering_camera(camera),
    _center(center),
    _radius(radius),
    _lookat_point(lookat)
    {
    }
    
    circle_controller() {}
    
    void update()
    {
        static constexpr float PI =3.14159265358979f;
        static constexpr float step = 2.0f * PI / 100.0f;
        
        float x = _center.x + _radius * std::cosf(_theta);
        float z = _center.z + _radius * std::sinf(_theta);
        
        _rendering_camera->position = glm::vec3(x,_center.y, z );
        _rendering_camera->forward = glm::normalize(_lookat_point -_rendering_camera->position);
        
        _rendering_camera->update_view_matrix();
        
        _theta += step;
    }
    
private:
    float _radius = 0.0f;
    float _theta = 0.0f;
    
    glm::vec3 _lookat_point = glm::vec3(0.0f);
    glm::vec3 _center = glm::vec3(0.0f);
    vk::camera * _rendering_camera = nullptr;
};


