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
#include "EASTL/fixed_string.h"

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
        
        
        void set_enable_mipmapping(bool b)
        {
            for( int i = 0; i < elements.size(); ++i)
            {
                elements[i].set_enable_mipmapping(b);
            }
        }
        
        void set_name(const char* name)
        {
            _name.clear();
            _name = name;
        }
        
        auto& get_name()
        {
            return _name;
        }
        
        void set_multisampling(bool b)
        {
            for( int i = 0; i < elements.size(); ++i)
            {
                elements[i].set_multisampling(b);
            }
        }
        
        
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
        
        inline void set_native_layout( image::image_layouts l)
        {
            for( int i = 0; i < elements.size(); ++i)
            {
                elements[i].set_native_layout(l);
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
        
        inline bool is_multisampling()
        {
                return elements[0].is_multisampling();
        }
        
        inline void reset_image_layout()
        {
            for( int i = 0; i < elements.size(); ++i)
            {
                if(elements[i].is_initialized())
                    elements[i].reset_image_layout();
            }
        }
        
        
        eastl_size_t size(){ return elements.size(); }
        
        virtual void destroy() override
        {
            private_destroy();
        }
        
    private:
        
        eastl::fixed_string<char, 50> _name = {};
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
        
        void set_name(const char* name)
        {
            _name.clear();
            _name = name;
        }
        
        auto& get_name()
        {
            return _name;
        }
        
        inline T*& operator[](int i) { return elements[i]; }
        
        eastl_size_t size(){ return elements.size(); }
        
        static char const * const *  get_class_type(){ return (&_resource_type); }
        
        virtual char const * const * get_instance_type() override
        {
            //note: the need for need arises because resource_set<image> does not return the right type,
            //this if statements will guarantee correct indentity, the expense of maintenance...
            if( elements[0]->get_instance_type() == depth_texture::get_class_type())
            {
                return resource_set<depth_texture*>::get_class_type();
            }
            else if( elements[0]->get_instance_type() == glfw_present_texture::get_class_type())
            {
                return resource_set<glfw_present_texture*>::get_class_type();
            }
            else if( elements[0]->get_instance_type() == texture_3d::get_class_type())
            {
                return resource_set<texture_3d*>::get_class_type();
            }
            else if( elements[0]->get_instance_type() == texture_2d::get_class_type())
            {
                return resource_set<texture_2d*>::get_class_type();
            }
            else if( elements[0]->get_instance_type() == render_texture::get_class_type())
            {
                return resource_set<render_texture*>::get_class_type();
            }
            else
            {
                assert(0 && "unrecognized asset");
            }
            
            
            return nullptr;
            
        };
        
        inline void set_native_layout( image::image_layouts l)
        {
            for( int i = 0; i < elements.size(); ++i)
            {
                elements[i]->set_native_layout(l);
            }
        }
        
        void init()
        {
            for( int i = 0; i < elements.size(); ++i)
            {
                elements[i]->init();
            }
        }
        
        inline bool is_multisampling()
        {
            return elements[0]->is_multisampling();
        }
        
        void set_multisampling(bool b)
        {
            for( int i = 0; i < elements.size(); ++i)
            {
                elements[i]->set_multisampling(b);
            }
        }
        
        void set_enable_mipmapping(bool b)
        {
            for( int i = 0; i < elements.size(); ++i)
            {
                elements[i]->set_enable_mipmapping(b);
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
        
        inline void reset_image_layout()
        {
            for( int i = 0; i < elements.size(); ++i)
            {
                elements[i]->reset_image_layout();
            }
        }
        
        virtual void destroy() override
        {
            private_destroy();
        }
        
    private:
        
        static constexpr char const * _resource_type = nullptr;
        eastl::array<T*, NUM_SWAPCHAIN_IMAGES> elements {};
        
        eastl::fixed_string<char, 50> _name = {};
        
        void private_destroy()
        {
            for( int i = 0; i < NUM_SWAPCHAIN_IMAGES; ++i)
            {
                elements[i]->destroy();
            }
        }
    };

}
