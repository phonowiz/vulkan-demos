//
//  node.hpp
//  vulkan-demos
//
//  Created by Rafael Sabino on 1/18/20.
//  Copyright © 2020 Rafael Sabino. All rights reserved.
//

#pragma once

#include <array>


namespace  vk {
    
    template<uint32_t NUM_CHILDREN>
    class node
    {
    public:
        node(){}
        virtual void validate() = 0;
        virtual void build() = 0;
        virtual void execute() = 0;
        virtual void get_pipeline() = 0;
        
        const char* name = nullptr;
    private:
        
        std::array<node, NUM_CHILDREN> children {};
    };
}
