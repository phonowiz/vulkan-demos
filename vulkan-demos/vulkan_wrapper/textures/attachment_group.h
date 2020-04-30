//
//  attachment_set.hpp
//  vulkan-demos
//
//  Created by Rafael Sabino on 3/19/20.
//  Copyright © 2020 Rafael Sabino. All rights reserved.
//

#pragma once

#include "image.h"
#include "glfw_swapchain.h"
#include "depth_texture.h"
#include "render_texture.h"
#include "texture_2d.h"
#include "glfw_present_texture.h"
#include "resource_set.h"

#include <iostream>
#include "EASTL/array.h"
#include <type_traits>
#include <glm/glm.hpp>

namespace vk {
    

//    using image_set = eastl::array<image*, vk::glfw_swapchain::NUM_SWAPCHAIN_IMAGES>;
//    using texture_2d_set = eastl::array<texture_2d, glfw_swapchain::NUM_SWAPCHAIN_IMAGES>;
//    using depth_set = eastl::array<depth_texture, glfw_swapchain::NUM_SWAPCHAIN_IMAGES>;
//    using render_texture_set = eastl::array< render_texture, glfw_swapchain::NUM_SWAPCHAIN_IMAGES>;
//    using glfw_present_texture_set = eastl::array< glfw_present_texture, glfw_swapchain::NUM_SWAPCHAIN_IMAGES>;


    template< uint32_t NUM_ATTACHMENTS>
    class attachment_group : public object
    {
        
//        using image_set = east::array<vk::image, glfw_swapchain::NUM_SWAPCHAIN_IMAGES>;
        
    public:
        
        //note: no copying of groups
        attachment_group & operator=(const attachment_group&) = delete;
        attachment_group(const attachment_group&) = delete;
        attachment_group & operator=(const attachment_group) = delete;
        attachment_group & operator=(attachment_group&) = delete;
        attachment_group(attachment_group&) = delete;
        
        static constexpr uint32_t MAX_NUMBER_OF_ATTACHMENTS = 10u;
        
        static_assert(MAX_NUMBER_OF_ATTACHMENTS > (NUM_ATTACHMENTS ));
        
        attachment_group(){}
        attachment_group(device* device, glm::vec2 dimensions){ _device = device; _dimensions = dimensions; };
        
//        inline void add_attachment(texture_2d_set& textures_set)
//        {
//            assert(num_attachments < NUM_ATTACHMENTS );
//            for( int i = 0; i < textures_set.size(); ++i)
//            {
//                _attachments[num_attachments][i] = static_cast<image*>( &textures_set[i] );
//                _attachments[num_attachments][i]->set_device(_device);
//                assert(_attachments[num_attachments][i]->get_width() == _dimensions.x && _attachments[num_attachments][i]->get_height() == _dimensions.y &&
//                       "the attachment dimensions are not compatible with this attachment group");
//            }
//            ++num_attachments;
//        }
        
        inline void set_dimensions(glm::vec2 v)
        {
            _dimensions = v;
        }
        
        inline int32_t get_depth_set_index()
        {
            int32_t result = -1;
            for( int i = 0; i < _attachments.size(); ++i)
            {
                if( _attachments[i].get_instance_type() == resource_set<depth_texture*>::get_class_type())
                {
                    result = i;
                    break;
                }
            }
            return result;
        }
        
        inline resource_set<image*>* get_depth_set(){
            
            
            //return &(_attachments[NUM_ATTACHMENTS-1]);
            
            resource_set<image*>* result = nullptr;
                
            int32_t i = get_depth_set_index();
            assert(i != -1);
            result = &(_attachments[i]);
                
            return result;
        }
        
//        inline void set_depth_set(resource_set<depth_texture>& textures_set)
//        {
//            for( int i = 0; i < textures_set.size(); ++i)
//            {
//                //note: the rule in this code is: depth attachment is always the last one
//                _attachments[NUM_ATTACHMENTS][i] = static_cast<image*>( &textures_set[i] );
//                _attachments[NUM_ATTACHMENTS][i]->set_device(_device);
//                _attachments[NUM_ATTACHMENTS][i]->set_dimensions(_dimensions.x, _dimensions.y, 1);
//
//            }
//        }
        
        inline void add_attachment(resource_set<depth_texture>& depth_set, float default_depth, float default_stencil)
        {
            _clear_values[num_attachments] = { default_depth, default_stencil};
            add_attachment(depth_set);
        }
        
        template< typename R>
        inline void add_attachment(resource_set<R>& textures_set, glm::vec4 clear_color)
        {
            _clear_values[num_attachments] = { clear_color.x, clear_color.y, clear_color.z, clear_color.w};
            
            add_attachment(textures_set);
        }
        
//        template<>
//        inline void add_attachment<depth_texture>(resource_set<depth_texture>& textures_set)
//        {
//            set_depth_set(textures_set);
//        }
        
//        inline void add_attachment(glfw_present_texture_set& textures_set)
//        {
//            assert(num_attachments < NUM_ATTACHMENTS );
//            for( int i = 0; i < textures_set.size(); ++i)
//            {
//                _attachments[num_attachments][i] = static_cast<image*>( &textures_set[i] );
//                _attachments[num_attachments][i]->set_device(_device);
//                _attachments[num_attachments][i]->set_dimensions(_dimensions.x, _dimensions.y, 1);
//            }
//            ++num_attachments;
//        }
        
        //TODO: REMOVE THIS FUNCTION, USE RENDER SETS INSTEAD
        inline void set_filter( image::filter filter)
        {
            assert(0 && "do not use");
            //assert(attachment_id < num_attachments);
            for( int attachment_id = 0; attachment_id < _attachments.size(); ++attachment_id )
            {
                for( int i = 0; i < _attachments[attachment_id].size(); ++i)
                {
                    //filters don't apply to present textures, they are never sampled by shaders
                    if(resource_set<glfw_present_texture>::get_class_type() !=
                       _attachments[attachment_id][i]->get_instance_type())
                    {
                        _attachments[attachment_id][i]->set_filter(filter);
                    }
                }
            }
        }
        
        //TODO: REMOVE THIS FUNCTION, USE RENDER SETS INSTEAD
        inline void set_filter(uint32_t attachment_id, image::filter filter)
        {
            assert(0 && "dont use this function");
            assert(attachment_id < num_attachments);
            for( int i = 0; i < _attachments[attachment_id].size(); ++i)
            {
                _attachments[attachment_id][i]->set_filter(filter);
            }
        }
        
        //TODO: REMOVE THIS FUNCTION, USE RENDER SETS INSTEAD
        inline void set_format(image::formats format)
        {
            assert(0 && "dont use this function");
            for( int i = 0; i < _attachments.size(); ++i)
            {
                set_format(i, format);
            }
        }
        
        //TODO: REMOVE THIS FUNCTION, USE RENDER SETS INSTEAD
        inline void set_format(uint32_t attachment_id, image::formats format)
        {
            assert(0 && "dont use this function");
            assert(num_attachments < NUM_ATTACHMENTS);
            for( int i = 0; i < _attachments[attachment_id].size(); ++i)
            {
                _attachments[attachment_id][i]->set_format(format);
            }
        }
        
        inline void init(uint32_t swapchain_id)
        {
            assert(num_attachments ==NUM_ATTACHMENTS && "you haven't populated all attachments");
            //note: here we don't initialize the depth texture, render passes decide that
            for( int i = 0; i < NUM_ATTACHMENTS; ++i)
            {
                //note: present textures are special and need not need to be initialized here, they are initialized
                //using libraries such as glfw that talk to the OS
                
                if(_attachments[i][swapchain_id]->get_instance_type() != glfw_present_texture::get_class_type())
                    //TODO: ATTACHMENTS SHOULD BE INITTED BEFORE WE GET HERE...
                    _attachments[i][swapchain_id]->init();
            }
        }
        
        
        inline void set_device(device* device)
        {
//            assert(num_attachments == NUM_ATTACHMENTS && "you must have all attachments before trying to set their devices");
            _device = device;
//            for( int i = 0; i < NUM_ATTACHMENTS; ++i)
//            {
//                for( int j = 0; j < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++j)
//                {
//                    _attachments[i][j]->set_device(device);
//                }
//            }
        }
        
        
        VkClearValue* get_clear_values() { return _clear_values.data(); }
        
        uint32_t size() { return num_attachments; }
        
        inline resource_set<vk::image*>& operator[](int i) { return _attachments[i]; }
        
        virtual void destroy() override
        {
            for( int i = 0; i < _attachments.size(); ++i)
            {
                for( int j = 0; j < _attachments[i].size(); ++i)
                {
                    _attachments[i][j]->destroy();
                }
            }
            
        }
        
    private:
        
        
        template< typename R>
        inline void add_attachment(resource_set<R>& textures_set)
        {
            assert(textures_set.get_instance_type() != resource_set<texture_2d>::get_class_type() && "texture 2d's are pre initialized since they are loaded from hard drive, "
                   "did you mean to use a render_texture instead?");
            assert(num_attachments < NUM_ATTACHMENTS );
            
//            if( textures_set.get_instance_type() != resource_set<depth_texture>::get_class_type())
//            {
//                set_depth_enble(true);
//            }
            for( int i = 0; i < textures_set.size(); ++i)
            {
                _attachments[num_attachments][i] = static_cast<image*>( &textures_set[i] );
                _attachments[num_attachments][i]->set_device(_device);
                //_attachments[num_attachments][i]->set_dimensions(_dimensions.x, _dimensions.y, 1);
            }
            ++num_attachments;
        }
        
//        inline void set_dimensions(uint32_t width, uint32_t height)
//        {
//            _dimensions.x = static_cast<float>(width);
//            _dimensions.y = static_cast<float>(height);
//            
//            assert(num_attachments == NUM_ATTACHMENTS && "you must have all attachments before trying to set all of their dimensions");
//            for( int i = 0; i < MAX_NUM_ATTACHMENTS; ++i)
//            {
//                for( int j = 0; j < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++j)
//                {
//                    if(_attachments[i][j]->get_instance_type() == glfw_present_texture::get_class_type() ||
//                       _attachments[i][j]->get_instance_type() == depth_texture::get_class_type())
//                    {
//                        //note: you cannot change present textures width/height/depth, and other textures must
//                        //match these dimensions if this is an attachment to a render pass
//                        assert(_attachments[i][j]->get_width() == width);
//                        assert(_attachments[i][j]->get_height() == height);
//                    }
//                    //note: as far I know, you cant render to 3d attachments yet, I could be wrong...
//                    _attachments[i][j]->set_dimensions(width, height, 1);
//                }
//            }
//        }
        
        eastl::array<VkClearValue,NUM_ATTACHMENTS> _clear_values {};
        
        //note: we add 1 to accomodate for the depth attachment
        device* _device = nullptr;
        
        glm::vec2 _dimensions = glm::vec2(0.0f, 0.0f);
        eastl::array<resource_set<vk::image*>, NUM_ATTACHMENTS> _attachments = {};
        uint32_t num_attachments = 0;
    };
}
