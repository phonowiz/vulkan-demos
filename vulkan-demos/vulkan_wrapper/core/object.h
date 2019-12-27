//
//  object.hpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 3/5/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#pragma once


namespace vk
{
    class object
    {
        
    public:
        
        object(){};
        virtual void destroy() = 0;
        virtual ~object(){}
    };
}



