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

class first_person_controller {
public:
    
    
    first_person_controller(vk::camera * camera, GLFWwindow* window):
    _mouse_x_start(0.0f),
    _mouse_y_start(0.0f)
    {
        _window = window;
        _target_camera = new vk::perspective_camera();
        _rendering_camera = camera;
        
    }
    
    void lock(bool b){ _lock = b;}
    first_person_controller() { delete _target_camera; }
    
    void update();
    void reset(){ _first_update = true; }
private:
    bool _first_update = true;
    
    static constexpr float CAMERA_SPEED = 1.4f;
    static constexpr float CAMERA_ROTATION_SPEED = 0.005f;
    static constexpr float CAMERA_POSITION_INTERPOLATION_SPEED = 10.0f;
    static constexpr float CAMERA_ROTATION_INTERPOLATION_SPEED = 12.0f;
    
    vk::camera * _rendering_camera;
    vk::camera * _target_camera; // Dummy camera used for interpolation.
    GLFWwindow* _window = nullptr;
    
    double _time = 0.0;
    double _delta_time = 0.0;
    
    bool _lock = false;
    float _mouse_x_start;
    float _mouse_y_start;
};

