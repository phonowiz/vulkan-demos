//
//  camera.hpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 7/21/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_SILENT_WARNINGS

#include <glm/gtc/matrix_transform.hpp>

namespace vk
{
    class camera {
        
    public:
        
        glm::vec3 up = { 0.0f,1.0f,0.0f };
        glm::vec3 forward = { 0.0f,0.0f,-1.f };
        glm::vec3 position = { 0.f,0.f,0.f };
        
        glm::mat4 view_matrix;
        
        const glm::mat4 & get_projection_matrix() const;
        void set_projection_matrix(glm::mat4 projection_matrix);
        
        virtual void update_view_matrix();
        
        inline glm::vec3 right() { return glm::normalize(-glm::cross(up, forward)); }
        inline glm::vec3 front() { return glm::normalize(forward); }
        
        
        inline float get_fov() const {return _fov;}
        inline float get_aspect_ratio() const { return _aspect;}
        inline float get_near() const {return _near;}
        inline float get_far() const {return _far;}
        inline float get_focal_length() const {return _focal_length;}
        
        camera(){}
        
    protected:
        
        bool _projection_matrix_has_changed = true;
        bool _dirty_projection_matrix = true;
        
        camera(glm::mat4 projection_matrix) : _projection_matrix(projection_matrix),
        _fov(0.0f),
        _aspect(0.0f),
        _near(0.0f),
        _far(0.0f),
        _focal_length(0.0f)
        {
        }
        
        camera(float _fov, float _aspect, float _near, float _far) :
        _projection_matrix(glm::mat4(1)),
        _fov(_fov),
        _aspect(_aspect),
        _near(_near),
        _far(_far),
        _focal_length(0.0f)
        {
        }
        
        glm::mat4 _projection_matrix;
        float _fov;
        float _aspect;
        float _near;
        float _far;
        float _focal_length;
    };

}
