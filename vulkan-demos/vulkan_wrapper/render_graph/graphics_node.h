//
//  graphics_node.hpp
//  vulkan-demos
//
//  Created by Rafael Sabino on 1/18/20.
//  Copyright © 2020 Rafael Sabino. All rights reserved.
//

#pragma once

#include "node.h"
#include "graphics_pipeline.h"

namespace vk {

    template<class RENDER_TEXTURE_TYPE, uint32_t NUM_ATTACHMENTS, uint32_t NUM_CHILDREN =10u>
    class graphics_node : public vk::node<NUM_CHILDREN>
    {
    public:
        using type_graphics_pipeline = graphics_pipeline<RENDER_TEXTURE_TYPE, NUM_ATTACHMENTS> ;
        
        virtual void validate(){};
        virtual void build() {};
        virtual void execute(){};
        
        virtual type_graphics_pipeline& get_pipeline(){return _pipeline; };
        
        void connect_subpass(node&, const char* parameter_name, uint32_t source, uint32_t destination);
        virtual void add_pass(node& pass);
        
    private:
    
        type_graphics_pipeline _pipeline;
    };

    #include "graphics_node.hpp"
}
