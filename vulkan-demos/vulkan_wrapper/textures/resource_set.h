//
//  resource_set.cpp
//  vulkan-demos
//
//  Created by Rafael Sabino on 4/15/20.
//  Copyright © 2020 Rafael Sabino. All rights reserved.
//

#pragma once

#include "glfw_present_texture.h"
#include "depth_texture.h"
#include "texture_3d.h"
#include "texture_2d.h"
#include "render_texture.h"

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
        
        virtual char const * const * get_instance_type() override
        {
            //note: the need for need arises because resource_set<image> does not return the right type,
            //this if statements will guarantee correct indentity, the expense of maintenance...
            if( elements[0].get_instance_type() == depth_texture::get_class_type())
            {
                return resource_set<depth_texture>::get_class_type();
            }
            else if( elements[0].get_instance_type() == glfw_present_texture::get_class_type())
            {
                return resource_set<glfw_present_texture>::get_class_type();
            }
            else if( elements[0].get_instance_type() == texture_3d::get_class_type())
            {
                return resource_set<texture_3d>::get_class_type();
            }
            else if( elements[0].get_instance_type() == texture_2d::get_class_type())
            {
                return resource_set<texture_2d>::get_class_type();
            }
            else if( elements[0].get_instance_type() == render_texture::get_class_type())
            {
                return resource_set<render_texture>::get_class_type();
            }
            else
            {
                assert(0 && "unrecognized asset");
            }
            
            
            return (&_resource_type);
            
        };
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
        
        inline void set_format(image::formats format)
        {
            for( int i = 0; i < elements.size(); ++i)
            {
                elements[i].set_format(format);
            }
        }
        inline void set_filter( image::filter filter)
        {
            for( int i = 0; i < elements.size(); ++i)
            {
                elements[i].set_filter(filter);
            }
        }
        
//        template<>
//        void resource_set<glfw_present_texture>::set_filter( image::filter)
//        {
//            //present textures don't need filters, they are not to be used for
//            //shader samplers
//        }
        
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
        
        virtual char const * const * get_instance_type() override
        {
            //note: the need for need arises because resource_set<image> does not return the right type,
            //this if statements will guarantee correct indentity, the expense of maintenance...
            if( elements[0]->get_instance_type() == depth_texture::get_class_type())
            {
                return resource_set<depth_texture>::get_class_type();
            }
            else if( elements[0]->get_instance_type() == glfw_present_texture::get_class_type())
            {
                return resource_set<glfw_present_texture>::get_class_type();
            }
            else if( elements[0]->get_instance_type() == texture_3d::get_class_type())
            {
                return resource_set<texture_3d>::get_class_type();
            }
            else if( elements[0]->get_instance_type() == texture_2d::get_class_type())
            {
                return resource_set<texture_2d>::get_class_type();
            }
            else if( elements[0]->get_instance_type() == render_texture::get_class_type())
            {
                return resource_set<render_texture>::get_class_type();
            }
            else
            {
                assert(0 && "unrecognized asset");
            }
            
            
            return nullptr;
            
        };
        
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
        
        inline void set_format(image::formats format)
        {
            for( int i = 0; i < elements.size(); ++i)
            {
                elements[i]->set_format(format);
            }
        }
        
        void set_filter( image::filter filter)
        {
            for( int i = 0; i < elements.size(); ++i)
            {
                elements[i]->set_filter(filter);
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
