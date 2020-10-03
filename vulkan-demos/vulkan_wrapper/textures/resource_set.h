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
#include "texture_cube.h"
#include "render_texture.h"

#include "EASTL/array.h"
#include "EASTL/fixed_string.h"
#include "EASTL/queue.h"
#include "EASTL/stack.h"

#include <iostream>

namespace vk
{
    static constexpr int NUM_SWAPCHAIN_IMAGES = 3;

    struct usage_transition
    {
        image::image_layouts previous;
        image::image_layouts current;
        vk::usage_type current_usage_type;
    };


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
        
        inline void log_transition(vk::usage_type l)
        {
            usage_transition trans {};
            trans.previous = elements[0].get_original_layout();
            trans.current_usage_type = l;
            if(!_layout_queue.empty())
            {
                trans.previous = _layout_queue.back().current;
            }
            trans.current = elements[0].get_usage_layout(l);
            EA_ASSERT_FORMATTED(!(trans.current == image::image_layouts::UNDEFINED || trans.current == image::image_layouts::PREINITIALIZED), ("invalid transition for texture %s\n", _name.c_str()));
            
            _layout_queue.push(trans);
            //std::cout << _name.c_str() << ": history count:" << _layout_queue.size() << std::endl;
            EA_ASSERT( !_layout_queue.empty());
        }
        
        inline usage_transition get_last_transition()
        {
            EA_ASSERT( !_layout_queue.empty() );
            usage_transition r = _layout_queue.back();
            return r;
        }
        
        inline usage_transition get_current_transition()
        {
            EA_ASSERT_FORMATTED( !_layout_queue.empty(), ("there are no transitions available for %s", _name.c_str()));
            usage_transition r = _layout_queue.front();
            return r;
        }
        inline void pop_transition()
        {
            EA_ASSERT_MSG((_layout_queue.size() != 0 ), "popping layout queue that is empty" );
            usage_transition r = _layout_queue.front();
            _layout_queue.pop();
            
            //std::cout << "popping: " <<  _name.c_str() << " count: " << _layout_queue.size() << std::endl;
            _used_transitions.push(r);
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
        
        glm::vec3 get_dimensions()
        {
            return elements[0].get_dimensions();
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
            else if( elements[0].get_instance_type() == texture_cube::get_class_type())
            {
                return resource_set<texture_cube>::get_class_type();
            }
            else
            {
                EA_FAIL_MSG("unrecognized asset");
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
        
        
        inline void set_original_layout( image::image_layouts l)
        {
            for( int i = 0; i < elements.size(); ++i)
            {
                elements[i].set_original_layout(l);
            }
        }
        
        inline void set_native_layout( image::image_layouts l)
        {
            for( int i = 0; i < elements.size(); ++i)
            {
                elements[i].set_native_layout(l);
            }
        }
        
        inline void change_layout(image::image_layouts l)
        {
            for( int i = 0; i < elements.size(); ++i)
            {
                elements[i].change_layout(l);
            }
        }
        
        inline void init_layout( vk::usage_type usage)
        {
            image::image_layouts l = get_usage_layout(usage);
            set_native_layout(l);
        }
        
        inline image::image_layouts get_usage_layout( vk::usage_type usage)
        {
            return elements[0].get_usage_layout(usage);
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
        
        inline void reset_image_layout(uint32_t i)
        {
            EA_ASSERT(_layout_queue.empty());
            while(!_used_transitions.empty())
            {
                _layout_queue.push( _used_transitions.front() );
                _used_transitions.pop();
            }
        }
        
        
        eastl_size_t size(){ return elements.size(); }
        
        virtual void destroy() override
        {
            private_destroy();
        }
        
        //this is to allow assignment operator to this type
        friend resource_set<image*>;
    private:
        
        eastl::fixed_string<char, 50> _name = {};
        eastl::array<T, NUM_SWAPCHAIN_IMAGES> elements {};
        eastl::queue<usage_transition> _layout_queue;
        eastl::queue<usage_transition> _used_transitions;
        
        void private_destroy()
        {
            for( int i = 0; i < NUM_SWAPCHAIN_IMAGES; ++i)
            {
                elements[i].destroy();
            }
        }

        static constexpr char const * _resource_type = nullptr;
    };

    //TODO: let's see if we can get rid of this class, we basically duplicates the template class above
    //specialize the resource_set class for pointers
    template< typename T>
    class resource_set<T*> : public object
    {
        
    public:
        
        inline void log_transition(vk::usage_type l)
        {
            usage_transition trans {};
            trans.current_usage_type = l;
            
            trans.previous = elements[0].get_original_layout();
            if(!_layout_queue.empty())
            {
                trans.previous = _layout_queue.front().current;
            }
            trans.current = elements[0].get_usage_layout(l);
            _layout_queue.push(trans);
        }
        
        inline bool has_transitions()
        {
            return !_layout_queue.empty();
        }
        inline usage_transition get_last_transition_added()
        {
            return _layout_queue.back();
        }
        
        inline usage_transition get_current_transition()
        {
            EA_ASSERT( !_layout_queue.empty() );
            usage_transition r = _layout_queue.front();
            return r;
        }
        inline void pop_transition()
        {
            usage_transition r = _layout_queue.front();
            _layout_queue.pop();
            //std::cout << "count %i :" << _name.c_str() << std::endl;
            _used_transitions.push(r);
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
        
        inline T*& operator[](int i) { return elements[i]; }
        
        eastl_size_t size(){ return elements.size(); }
        
        template < typename NEW_T >
        resource_set<image*>& operator=(  resource_set< NEW_T > &rhs )
        {
            for( int i = 0; i < elements.size(); ++i)
            {
                elements[i] = static_cast<image*>(&rhs[i]);
            }
            
            _layout_queue = rhs._layout_queue;
            _used_transitions = rhs._used_transitions;
            return *this;
        }

        template < typename NEW_T >
        resource_set<image*>& operator=(   NEW_T &rhs )
        {
            //note: here we are assigning a single texture to
            //a resource_set, which means that for every frame in flight we are going
            //to be using the same texture, this could cause issues if we start transitioning
            //a texture while is being read in different threat in the GPU.
            
            for( int i = 0; i < elements.size(); ++i)
            {
                elements[i] = static_cast<image*>(&rhs);
            }
            return *this;
        }
        
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
            else if( elements[0]->get_instance_type() == texture_cube::get_class_type())
            {
                return resource_set<texture_cube>::get_class_type();
            }
            else
            {
                EA_FAIL_MSG("unrecognized asset");
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
        
        inline void change_layout(image::image_layouts l)
        {
            for( int i = 0; i < elements.size(); ++i)
            {
                elements[i]->change_layout(l);
            }
        }
        
        inline void init_layout( vk::usage_type usage)
        {
            image::image_layouts l = get_usage_layout(usage);
            set_native_layout(l);
        }
        
        
        image::image_layouts get_usage_layout( vk::usage_type usage)
        {
            return elements[0]->get_usage_layout(usage);
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
        
        glm::vec3 get_dimensions()
        {
            return elements[0]->get_dimensions();
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
        
        inline void reset_image_layout(uint32_t i)
        {
            EA_ASSERT(_layout_queue.empty());
            while(!_used_transitions.empty())
            {
                _layout_queue.push( _used_transitions.front() );
                _used_transitions.pop();
            }
        }
        
        virtual void destroy() override
        {
            private_destroy();
        }
        
    private:
        
        static constexpr char const * _resource_type = nullptr;
        eastl::array<T*, NUM_SWAPCHAIN_IMAGES> elements {};
        eastl::queue<usage_transition> _layout_queue;
        eastl::queue<usage_transition> _used_transitions;
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
