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

//#include "Time/FrameRate.h"
//#include "Graphic/Camera/PerspectiveCamera.h"
//#include "Application.h"
#define GLFW_INCLUDE_VULKAN



#include <GLFW/glfw3.h>
#include "../vulkan_wrapper/cameras/camera.h"
#include "vulkan_wrapper/cameras/perspective_camera.h"

class first_person_controller {
public:
    
    
    first_person_controller(vk::camera * camera, GLFWwindow* window):
    mouseXStart(0.0f),
    mouseYStart(0.0f)
    {
        _window = window;
        targetCamera = new vk::perspective_camera();
        renderingCamera = camera;
        
    }
    
    first_person_controller() { delete targetCamera; }
    
    void update();
private:
    bool firstUpdate = true;
    
    const float CAMERA_SPEED = 1.4f;
    const float CAMERA_ROTATION_SPEED = 0.003f;
    const float CAMERA_POSITION_INTERPOLATION_SPEED = 8.0f;
    const float CAMERA_ROTATION_INTERPOLATION_SPEED = 8.0f;
    
    vk::camera * renderingCamera;
    vk::camera * targetCamera; // Dummy camera used for interpolation.
    GLFWwindow* _window = nullptr;
    
    float mouseXStart;
    float mouseYStart;
};

