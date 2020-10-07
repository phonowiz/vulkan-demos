//
//  transform.cpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 7/28/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//


#pragma once

#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/transform.hpp"
#include "glm/mat4x4.hpp"
#include "glm/gtc/quaternion.hpp"

#include <ostream>

namespace vk {
    
    /// <summary> Represents a transform: rotation, position and scale. </summary>
    class transform {
    public:
        glm::vec3 position = { 0,0,0 }, scale = { 1,1,1 }, rotation = { 0,0,0 };
        transform * parent = nullptr;
        
        transform();
        /// <summary> Recalculates the transform matrix according to the position, scale and rotation vectors. </summary>
        void update_transform_matrix();
        
        void reset() { position = { 0,0,0 }; scale = { 1,1,1 }; rotation = { 0,0,0 }; }
        
        /// <summary> Returns a reference to the transform matrix </summary>
        const glm::mat4 & get_transform_matrix();
        
        /// <summary> Output. </summary>
        friend std::ostream & operator<<(std::ostream &, const transform &);
        
        // Bunch of helper functions.
        glm::vec3 forward();
        glm::vec3 up();
        glm::vec3 right();
        glm::mat4 mat_transform;
        
    private:
        
        /// <summary> Is true when the transform matrix is not correctly representing the position, scale and rotation vectors. </summary>
        bool transform_is_valid = false;

    };
}

