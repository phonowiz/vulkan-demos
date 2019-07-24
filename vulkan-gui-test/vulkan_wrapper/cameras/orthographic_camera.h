//
//  orthographic_camera.hpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 7/21/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#pragma once

#include "camera.h"


namespace  vk
{
    class orthographic_camera : public camera
    {
    public:
        
        //the default constructor of the orthographic camera creates OpenGL's orthographic canonical view and it is where clipping happens.
        //check out "Real-Time Rendering, Third Edition", section 4.6
        orthographic_camera();
        
        orthographic_camera(float view_space_width , float view_space_height, float view_space_depth);
        
        inline float get_left(){ return _left;}
        inline float get_right(){ return _right;}
        inline float get_bottom(){ return _bottom; }
        inline float get_top(){ return _top; }
        
    private:
        float _left;
        float _right;
        float _bottom;
        float _top;
    };

}
