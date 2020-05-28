//
//  first_person_controller.cpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 7/21/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#include "first_person_controller.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

void first_person_controller::update()
{
    if(_lock)
        return;
    double xpos, ypos;

    glfwGetCursorPos(_window, &xpos, &ypos);

    if (_first_update) {
        _target_camera->forward = _rendering_camera->forward;
        _target_camera->position = _rendering_camera->position;
        _first_update = false;

        _mouse_x_start = xpos;
        _mouse_y_start = ypos;
    }

    // ----------
    // Rotation.
    // ----------

    float xDelta = _mouse_x_start - xpos;
    float yDelta = _mouse_y_start - ypos;

    float xRot = static_cast<float>(CAMERA_ROTATION_SPEED * xDelta);
    float yRot = static_cast<float>(CAMERA_ROTATION_SPEED * yDelta);

    // X rotation.

    _target_camera->forward = glm::rotateY(_target_camera->forward, xRot);

    // Y rotation.
    glm::vec3 new_direction = glm::rotate(_target_camera->forward, yRot, _target_camera->right());
    float a = glm::dot(new_direction, glm::vec3(0, 1, 0));
    if (abs(a) < 0.99)
        _target_camera->forward = new_direction;


    double current_time = glfwGetTime();
    _delta_time = current_time - _time;
    
    _time = current_time;
    // ----------
    // Position.
    // ----------
    // Move forward.
    if (glfwGetKey(_window, GLFW_KEY_UP) == GLFW_PRESS || glfwGetKey(_window, GLFW_KEY_W) == GLFW_PRESS)
    {
        _target_camera->position += _target_camera->front() * (float)_delta_time * CAMERA_SPEED;
    }
    // Move backward.
    if (glfwGetKey(_window, GLFW_KEY_DOWN) == GLFW_PRESS || glfwGetKey(_window, GLFW_KEY_S) == GLFW_PRESS)
    {
        _target_camera->position -= _target_camera->front() * (float)_delta_time * CAMERA_SPEED;
    }
    // Strafe right.
    if (glfwGetKey(_window, GLFW_KEY_RIGHT) == GLFW_PRESS || glfwGetKey(_window, GLFW_KEY_D) == GLFW_PRESS)
    {
        _target_camera->position += _target_camera->right() * (float)_delta_time * CAMERA_SPEED;
    }
    // Strafe left.
    if (glfwGetKey(_window, GLFW_KEY_LEFT) == GLFW_PRESS || glfwGetKey(_window, GLFW_KEY_A) == GLFW_PRESS)
    {
        _target_camera->position -= _target_camera->right() * (float)_delta_time * CAMERA_SPEED;
    }

    // Interpolate between target and current camera.
    float rotation_interpolation= glm::clamp(_delta_time * CAMERA_ROTATION_INTERPOLATION_SPEED, 0.0, 1.0);
    float position_interpolation = glm::clamp(_delta_time * CAMERA_POSITION_INTERPOLATION_SPEED, 0.0, 1.0);

    //printf("%f\n", rotationInterpolation);
    _rendering_camera->forward = mix(_rendering_camera->forward, _target_camera->front(), rotation_interpolation);
    _rendering_camera->position = mix(_rendering_camera->position, _target_camera->position, position_interpolation);

    // Reset mouse position for next update iteration.
    //glfwSetCursorPos(window, xwidth / 2, yheight / 2);
    _mouse_x_start = xpos;
    _mouse_y_start = ypos;

    // Update view (camera) matrix.
    _rendering_camera->update_view_matrix();
}
