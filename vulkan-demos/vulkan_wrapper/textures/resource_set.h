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
        
        resource_set(){};
        
        resource_set & operator=(const resource_set&) = delete;
        resource_set(const resource_set&) = delete;
        resource_set & operator=(resource_set&) = delete;
        resource_set(resource_set&) = delete;
        
        virtual char const * const * get_instance_type() override { return (&_resource_type); };
        static char const * const *  get_class_type(){ return (&_resource_type); }
        
        inline T& operator[](int i) { return elements[i]; }
        
        
        void set_dimensions( uint32_t width, uint32_t height, uint32_t depth = 1)
        {
            for( int i = 0; i < elements.size(); ++i)
            {
                elements[i].set_dimensions(width, height, depth);
            }
        }
        
        void init()
        {
            for( int i = 0; i < elements.size(); ++i)
            {
                elements[i].init();
            }
        }
        
        void set_device( vk::device* dev)
        {
            for( int i = 0; i < elements.size(); ++i)
            {
                elements[i].set_device(dev);
            }
        }
        
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
        
        void init()
        {
            for( int i = 0; i < elements.size(); ++i)
            {
                elements[i]->init();
            }
        }
        void set_dimensions( uint32_t width, uint32_t height, uint32_t depth = 1)
        {
            for( int i = 0; i < elements.size(); ++i)
            {
                elements[i]->set_dimensions(width, height, depth);
            }
        }
        
        
        void set_device( vk::device* dev)
        {
            for( int i = 0; i < elements.size(); ++i)
            {
                elements[i]->set_device(dev);
            }
        }
        
        
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
