//
//  cornell_box.hpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 8/9/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#pragma once

#include "obj_shape.h"
#include <array>
namespace vk {
    
    class cornell_box : public obj_shape
    {
    public:
        cornell_box(vk::device* device );
        
        virtual void set_diffuse(glm::vec3 diffuse) override;
        virtual void create() override;
        
    private:
        
        std::array<glm::vec3, 7> _wall_colors;;
    };
}
