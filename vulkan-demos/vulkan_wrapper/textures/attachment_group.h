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

namespace vk
{


    template< uint32_t NUM_ATTACHMENTS>
    class attachment_group : public object
    {
        
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
        
        inline resource_set<image*>* get_depth_set()
        {
            resource_set<image*>* result = nullptr;
                
            int32_t i = get_depth_set_index();
            assert(i != -1);
            result = &(_attachments[i]);
                
            return result;
        }
        
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
        
        inline void init(uint32_t swapchain_id)
        {
            assert(num_attachments ==NUM_ATTACHMENTS && "you haven't populated all attachments");
        }
        
        
        inline void set_device(device* device)
        {
            _device = device;
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
            
            for( int i = 0; i < textures_set.size(); ++i)
            {
                _attachments[num_attachments][i] = static_cast<image*>( &textures_set[i] );
                _attachments[num_attachments][i]->set_device(_device);
            }
            ++num_attachments;
        }
        
        eastl::array<VkClearValue,NUM_ATTACHMENTS> _clear_values {};
        
        //note: we add 1 to accomodate for the depth attachment
        device* _device = nullptr;
        
        glm::vec2 _dimensions = glm::vec2(0.0f, 0.0f);
        eastl::array<resource_set<vk::image*>, NUM_ATTACHMENTS> _attachments = {};
        uint32_t num_attachments = 0;
    };
}
