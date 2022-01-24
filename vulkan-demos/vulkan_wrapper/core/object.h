//
//  object.hpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 3/5/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#pragma once

#include <EAAssert/eaassert.h>

namespace vk
{
    class object
    {
        
    public:
        
        object(){};
        
        virtual  char const * const * get_instance_type(){ EA_FAIL_MSG("this instance has no defined type"); return nullptr; };
        
        virtual void destroy() = 0;
        virtual ~object(){}
    };
}



