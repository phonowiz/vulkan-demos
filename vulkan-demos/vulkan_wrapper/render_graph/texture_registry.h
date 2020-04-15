//
//  resource_store.hpp
//  vulkan-demos
//
//  Created by Rafael Sabino on 4/4/20.
//  Copyright © 2020 Rafael Sabino. All rights reserved.
//

#pragma once

#include <map>
#include <memory>
#include <vector>
#include "image.h"
#include "render_pass.h"
#include "ordered_map.h"
#include "texture_2d.h"
#include "texture_3d.h"
#include "depth_texture.h"
#include "glfw_present_texture.h"
#include "command_recorder.h"
#include "EASTL/fixed_vector.h"
#include "EASTL/fixed_map.h"
#include "EASTL/fixed_string.h"
#include "resource_set.h"


namespace vk
{
    template< uint32_t NUM_CHILDREN_>
    class node;


    template<uint32_t NUM_CHILDREN>
    class texture_registry : public object
    {
        
    using node_type = vk::node<NUM_CHILDREN>;
    static constexpr size_t DEPENDENCIES_SIZE = 10;
    public:
        
        using image_ptr = std::shared_ptr<image>;
        using resource_ptr = std::shared_ptr<object>;
        
        struct dependee_data
        {
            node_type* node;
            resource_ptr resource = nullptr;
        };
        
        struct dependant_data
        {
            vk::image::image_layouts layout = {};
            dependee_data data = {};
        };
        
        using node_dependees  = eastl::fixed_vector<dependant_data, DEPENDENCIES_SIZE,true>;
        
        using node_dependees_map = eastl::fixed_map<vk::object*,node_dependees, DEPENDENCIES_SIZE, false>;
        
        using dependee_data_map = eastl::fixed_map<const char*, dependee_data, 50, true> ;
        
        
        texture_registry(){}
        
        inline node_dependees& get_dependees( node_type* node)
        {
            
            if(_node_dependees_map.find(node) == _node_dependees_map.end() )
            {
                _node_dependees_map[node] = node_dependees();
            }
            
            return _node_dependees_map[node];
        }
        
        
//        template <uint32_t NUM_ATTACHMENTS>
//        void set_write_attachment_group(const char* name, node_type* node, vk::attachment_group<NUM_ATTACHMENTS>& group)
//        {
//            static constexpr int MAX_STRING_SIZE = 250;
//            assert( MAX_STRING_SIZE > std::strlen(name));
//
//
//            assert (_dependee_data_map.find(name) == _dependee_data_map.end() && "this attachment name has already been chosen,"
//                                                                                  " pick a different one.");
//
//            dependee_data info {};
//            info.resource = std::static_pointer_cast<vk::object>(ptr);
//            info.node = node;
//
//            _dependee_data_map[name] = info;
//
////            for(int i = 0; i < NUM_ATTACHMENTS; ++i)
////            {
////                for( int s = 0 ; s < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++s)
////                {
////                    eastl::fixed_string<char, MAX_STRING_SIZE, true> str(name);
////                    str.append_sprintf("%i_%i", i, s);
////
////                    std::shared_ptr<texture_2d> tex = get_write_texture(str.c_str(), node, expected_layout );
////                    group[i][s] = tex;
////                }
////            }
//        }
        
//        template <uint32_t NUM_ATTACHMENTS>
//        void get_read_attachment_group(const char* name, node_type* node, vk::image::image_layouts expected_layout,
//                                  vk::attachment_group<NUM_ATTACHMENTS>& group)
//        {
//            static constexpr int MAX_STRING_SIZE = 250;
//            assert( MAX_STRING_SIZE > std::strlen(name));
//
//            for(int i = 0; i < NUM_ATTACHMENTS; ++i)
//            {
//                for( int s = 0 ; s < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++s)
//                {
//                    eastl::fixed_string<char, MAX_STRING_SIZE, true> str(name);
//                    str.append_sprintf("%i_%i", i, s);
//
//                    std::shared_ptr<texture_2d> tex = get_read_texture(str.c_str(), node, expected_layout );
//                    group[i][s] = tex;
//                }
//            }
//        }
        
        template <typename T>
        inline std::shared_ptr<T> get_read_texture(const char* name, node_type* node, vk::image::image_layouts expected_layout)
        {
            typename dependee_data_map::iterator iter = _dependee_data_map.find(name);
            
            std::shared_ptr<T> result = nullptr;
            if( iter != _dependee_data_map.end())
            {
                dependee_data d = iter->second;
                
                dependant_data dependant = {};
                dependant.data = d;
                dependant.layout = expected_layout;
                _node_dependees_map[node].push_back(dependant);
                result = std::static_pointer_cast<T>(d.resource);
            }
            
            return result;
        }
        
        inline resource_set<texture_2d>& get_write_texture_2d_set( const char* name, node_type* node )
        {
            return get_write_texture<resource_set<texture_2d>>(name, node);
        }
        
        inline resource_set<depth_texture>& get_write_depth_texture_set( const char* name, node_type* node )
        {
            return get_write_texture<resource_set<texture_2d>>(name, node);
        }
        
        inline resource_set<render_texture>& get_write_render_texture_set( const char* name, node_type* node )
        {
            return get_write_texture<resource_set<texture_2d>>(name, node);
        }
        
        inline resource_set<texture_3d>& get_write_texture_3d( const char* name, node_type* node )
        {
            return get_write_texture<resource_set<texture_3d>>(name, node);
        }

        
        inline vk::texture_2d& get_loaded_texture( const char* name, node_type* node, device* dev, const char* path )
        {
            typename dependee_data_map::iterator iter = _dependee_data_map.find(name);
            
            vk::texture_2d* result = nullptr;
            if( iter == _dependee_data_map.end())
            {
                result = &(get_write_texture<texture_2d>(name, node, dev, path));
                result->init();
            }
            else
                result = &(*(std::static_pointer_cast<texture_2d>(iter->second.resource)));
            
            return *result;
        }
        
        
        virtual void destroy() override
        {
            typename dependee_data_map::iterator b = _dependee_data_map.begin();
            typename dependee_data_map::iterator end = _dependee_data_map.end();
            while(b != end)
            {
                b->second.resource->destroy();
                ++b;
            }
        }
        
    private:
        
        
//        template<uint32_t NUM_ATTACHMENTS>
//        void set_write_attachment_group(const char* name, node_type* node, attachment_group<NUM_ATTACHMENTS>& group )
//        {
//            for(int i = 0; i < NUM_ATTACHMENTS; ++i)
//            {
//                for( int s = 0; s < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++i)
//                {
//                    dependee_data info {};
//                    info.image = std::static_pointer_cast<vk::image>(group[i][s]);
//                    info.obj = obj;
//
//                    _dependee_data_map[name] = info;
//                }
//
//            }
//        }
        

        template <typename T, typename ...ARGS>
        inline T& get_write_texture( const char* name, node_type* node,  ARGS... args)
        {
            
            assert (_dependee_data_map.find(name) == _dependee_data_map.end() && "you are asking for a texture already marked for writing."
                                                                        " Pick a different name");
            std::shared_ptr<T> ptr = GREATE_TEXTUE<T>(args...);
            
            dependee_data info {};
            info.resource = std::static_pointer_cast<vk::object>(ptr);
            info.node = node;

            _dependee_data_map[name] = info;
            
            return *ptr;
        }
        
        template <typename T, typename ...ARGS>
        inline static std::shared_ptr<T> GREATE_TEXTUE( ARGS... args)
        {
            return  std::make_shared<T> (args...);
        }
        
    private:
        
        node_dependees_map _node_dependees_map;
        dependee_data_map   _dependee_data_map;
    };
}
