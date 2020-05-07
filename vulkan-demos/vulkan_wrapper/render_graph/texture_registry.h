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
        
        texture_registry & operator=(const texture_registry&) = delete;
        texture_registry(const texture_registry&) = delete;
        texture_registry & operator=(texture_registry&) = delete;
        texture_registry(texture_registry&) = delete;
        
        using image_ptr = std::shared_ptr<image>;
        using resource_ptr = std::shared_ptr<object>;
        
        struct dependee_data
        {
            node_type* node;
            resource_ptr resource = nullptr;
            bool consumed = false;
        };
        
        struct dependant_data
        {
            vk::image::image_layouts layout = {};
            dependee_data data = {};
        };
        
        using node_dependees  = eastl::fixed_vector<dependant_data, DEPENDENCIES_SIZE,true>;
        
        using node_dependees_map = eastl::map<vk::object*,node_dependees>;
        
        using dependee_data_map = eastl::map< eastl::fixed_string<char, 100>, dependee_data> ;
        
        
        texture_registry(){}
        
        inline node_dependees& get_dependees( node_type* dependant_node)
        {
            
            if(_node_dependees_map.find(dependant_node) == _node_dependees_map.end() )
            {
                //note:: if I don't do this, returned value of the key will be garbage, something about
                //fixed maps causes this to happen.  I don't expect millions of dependent nodes here so it won't play a factor,
                //but a solution would great!
                _node_dependees_map.insert( dependant_node  );
            }
            
            return _node_dependees_map[static_cast<vk::object*>(dependant_node)];
        }
        
        
        inline resource_set<depth_texture>& get_read_depth_texture_set( const char* name, node_type* node, vk::usage_type usage_type)
        {
            std::shared_ptr< resource_set<depth_texture>> tex =  get_read_texture<resource_set<render_texture>>(name, node, usage_type);
            assert(tex != nullptr && " Invalid graph, the texture you depend on is not found");
            
            return *tex;
        }
        
        
        inline resource_set<render_texture>& get_read_render_texture_set( const char* name, node_type* node, vk::usage_type usage_type)
        {
            std::shared_ptr< resource_set<render_texture>> tex =  get_read_texture<resource_set<render_texture>>(name, node, usage_type);
            assert(tex != nullptr && " Invalid graph, the texture you depend on is not found");
            
            return *tex;
        }
        
        inline resource_set<texture_3d>& get_read_texture_3d_set( const char* name, node_type* node, vk::usage_type usage_type)
        {
            std::shared_ptr< resource_set<texture_3d>> tex =  get_read_texture<resource_set<texture_3d>>(name, node, usage_type);
            assert(tex != nullptr && " Invalid graph, the texture you depend on is not found");
            
            return *tex;
        }
        
        inline resource_set<texture_2d>& get_read_texture_2d_set( const char* name, node_type* node, vk::image::image_layouts expected_layout)
        {
            
            std::shared_ptr< resource_set<texture_2d>> tex =  get_read_texture<resource_set<texture_2d>>(name, node, expected_layout);
            
            assert(tex != nullptr && " Invalid graph, the texture you depend on is not found");
            
            return *tex;
        }
        
        inline resource_set<texture_2d>& get_write_texture_2d_set( const char* name, node_type* node, vk::usage_type usage_type )
        {
            resource_set<texture_2d>& result = get_write_texture<resource_set<texture_2d>>(name, node, usage_type);
            result.set_name(name);
            
            return get_write_texture<resource_set<texture_2d>>(name, node, usage_type);
        }
        
        inline resource_set<depth_texture>& get_write_depth_texture_set( const char* name, node_type* node, vk::usage_type usage_type)
        {
            resource_set<depth_texture>& result = get_write_texture<resource_set<depth_texture>>(name, node, usage_type);
            result.set_name(name);
            
            return result;
        }
        
        inline resource_set<render_texture>& get_write_render_texture_set( const char* name, node_type* node, vk::usage_type usage_type )
        {
            resource_set<render_texture>& result = get_write_texture<resource_set<render_texture>>(name, node, usage_type);
            result.set_name(name);
            return result;
        }
        
        //note: for texture_3d's the layout is always the same no matter the usage, this is why we don't pass in
        // a usage parameter
        inline resource_set<texture_3d>& get_write_texture_3d_set( const char* name, node_type* node )
        {
            resource_set<texture_3d>& result = get_write_texture<resource_set<texture_3d>>(name, node, vk::usage_type::STORAGE_IMAGE);
            result.set_name(name);
            return result;
        }

        
        inline vk::texture_2d& get_loaded_texture( const char* name, node_type* node, device* dev, const char* path )
        {
            typename dependee_data_map::iterator iter = _dependee_data_map.find(name);
            
            vk::texture_2d* result = nullptr;
            if( iter == _dependee_data_map.end())
            {
                result = &(get_write_texture<texture_2d>(name, node, vk::usage_type::COMBINED_IMAGE_SAMPLER, dev, path));
                result->init();
            }
            else
                result = &(*(std::static_pointer_cast<texture_2d>(iter->second.resource)));
            
            return *result;
        }
        
        bool is_resource_created(const char* name)
        {
            typename dependee_data_map::iterator iter = _dependee_data_map.find(name);
            
            return iter != _dependee_data_map.end();
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
        
        
        template<typename T>
        void make_dependency(T& type, dependee_data& d, node_type* node, vk::usage_type usage_type)
        {
            dependant_data dependant = {};
            dependant.data = d;
            dependant.layout = type.get_usage_layout(usage_type);
            _node_dependees_map[node].push_back(dependant);
        }
        
        template<typename T>
        void make_dependency(resource_set<T>& type, dependee_data& d, node_type* node, vk::usage_type usage_type)
        {
            dependant_data dependant = {};
            dependant.data = d;
            dependant.layout = type[0].get_usage_layout(usage_type);
            _node_dependees_map[node].push_back(dependant);
        }
        
        template <typename T>
        inline std::shared_ptr<T> get_read_texture(const char* name, node_type* node, vk::usage_type usage_type)
        {
            typename dependee_data_map::iterator iter = _dependee_data_map.find(name);
            
            std::shared_ptr<T> result = nullptr;
            if( iter != _dependee_data_map.end())
            {
                dependee_data& d = iter->second;
                
                result = std::static_pointer_cast<T>(d.resource);
                
                //TODO: add the name of the node here using EAASSERT
                assert( result != nullptr && "The asset this node depends on was not created, check the node which creates this asset");
                
                //eastl::fixed_string<char, 100> msg {};
                //msg.sprintf("accessing resource: %s", name);
                //node->debug_print(msg.c_str());
                
                d.consumed = true;
                
                make_dependency(*result, d, node, usage_type);
                
            }
            
            return result;
        }

        //template <typename T>
        template <typename T, typename ...ARGS>
        inline T& get_write_texture( const char* name, node_type* node,  vk::usage_type usage_type, ARGS... args)
        {
            typename dependee_data_map::iterator iter = _dependee_data_map.find(name);
            
            std::shared_ptr<T> ptr = nullptr;
            if(iter == _dependee_data_map.end())
            {
//                eastl::fixed_string<char, 100> msg {};
//                msg.sprintf("creating texture '%s'", name);
//                node->debug_print( msg.c_str());
                
                ptr = GREATE_TEXTUE<T>(args...);
                
                //TODO: do we need expected layout
                dependee_data info {};
                info.resource = std::static_pointer_cast<vk::object>(ptr);
                info.node = node;
                info.consumed = false;

                _dependee_data_map[name] = info;
                
                assert(_dependee_data_map.find(name) != _dependee_data_map.end());
            }
            else
            {
                ptr = std::static_pointer_cast<T>(iter->second.resource);
                
                dependee_data& d = iter->second;
                
                make_dependency(*ptr, d, node, usage_type);
                
                iter->second.consumed = false;
            }
            
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
