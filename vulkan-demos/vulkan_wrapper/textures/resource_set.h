//
//  resource_set.cpp
//  vulkan-demos
//
//  Created by Rafael Sabino on 4/15/20.
//  Copyright © 2020 Rafael Sabino. All rights reserved.
//

#pragma once

#include "EASTL/array.h"

namespace vk
{
    static constexpr int NUM_SWAPCHAIN_IMAGES = 3;


    //note: resource_set class was placed here to break up a cyclic dependency, ideally I'd like this
    //class to be with attachment_group, which is were resource_set was built for
    template< typename T >
    struct resource_set : public object
    {
    public:

        
        virtual char const * const * get_instance_type() override { return (&_resource_type); };
        static char const * const *  get_class_type(){ return (&_resource_type); }
        
        inline T& operator[](int i) { return elements[i]; }
        
        
        eastl_size_t size(){ return elements.size(); }
        
        virtual void destroy() override
        {
            private_destroy();
        }
        
    private:
            
        eastl::array<T, NUM_SWAPCHAIN_IMAGES> elements {};
        void private_destroy()
        {
            for( int i = 0; i < NUM_SWAPCHAIN_IMAGES; ++i)
            {
                elements[i].destroy();
            }
        }

        static constexpr char const * _resource_type = nullptr;
    };


    //specialize the resource_set class for pointers
    template< typename T>
    class resource_set<T*> : public object
    {
        
    public:
        
        inline T*& operator[](int i) { return elements[i]; }
        
        eastl_size_t size(){ return elements.size(); }
        
        virtual void destroy() override
        {
            private_destroy();
        }
        
    private:
        eastl::array<T*, NUM_SWAPCHAIN_IMAGES> elements {};
        void private_destroy()
        {
            for( int i = 0; i < NUM_SWAPCHAIN_IMAGES; ++i)
            {
                elements[i]->destroy();
            }
        }
    };

}
