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
#include "command_recorder.h"
#include "EASTL/fixed_vector.h"
#include "EASTL/fixed_map.h"

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
        
        struct dependee_data
        {
            vk::object* obj;
            image_ptr image = nullptr;
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
        
        inline node_dependees& get_dependees( vk::object* obj)
        {
            
            if(_node_dependees_map.find(obj) == _node_dependees_map.end() )
            {
                _node_dependees_map[obj] = node_dependees();
            }
            
            return _node_dependees_map[obj];
        }
        
        template <typename T, typename ...ARGS>
        inline image_ptr get_read_texture(const char* name, node_type* node, vk::image::image_layouts expected_layout)
        {
            typename dependee_data_map::iterator iter = _dependee_data_map.find(name);
            
            image_ptr result = nullptr;
            if( iter != _dependee_data_map.end())
            {
                dependee_data d = iter->second;
                
                dependant_data dependant = {};
                dependant.data = d;
                dependant.layout = expected_layout;
                _node_dependees_map[node].push_back(dependant);
                result = d.image;
            }
            
            return result;
        }
        
//        template<typename ARGS...>
//        inline std::shared_ptr<vk::texture_2d> get_write_texture( const char* name, node_type* node,  device* dev, ARGS... args)
//        {
//            std::shared_ptr<vk::texture_2d> tex = get_write_texture<vk::texture_2d>(name, node, args);
//            return get_write_texture<vk::texture_2d>(name, node, dev );
//        }
        
        inline std::shared_ptr<vk::texture_2d> get_loaded_texture( const char* name, node_type* obj, device* dev, const char* path )
        {
            typename dependee_data_map::iterator iter = _dependee_data_map.find(name);
            
            std::shared_ptr<vk::texture_2d> result = nullptr;
            if( iter == _dependee_data_map.end())
            {
                result = get_write_texture<texture_2d>(name, obj, dev, path);
                result->init();
            }
            else
                result = std::static_pointer_cast<texture_2d>(iter->second.image);
            
            return result;
        }
        
        
        virtual void destroy() override
        {
            typename dependee_data_map::iterator b = _dependee_data_map.begin();
            typename dependee_data_map::iterator end = _dependee_data_map.end();
            while(b != end)
            {
                b->second.image->destroy();
                ++b;
            }
        }
        
    private:
        
        template <typename T, typename ...ARGS>
        inline std::shared_ptr<T> get_write_texture( const char* name, node_type* obj,  ARGS... args)
        {
            
            assert (_dependee_data_map.find(name) == _dependee_data_map.end() && "you are asking for a texture already marked for writing."
                                                                        " Pick a different name");
            std::shared_ptr<T> ptr = GREATE_TEXTUE<T>(args...);
            
            dependee_data info {};
            info.image = std::static_pointer_cast<vk::image>(ptr);
            info.obj = obj;

            _dependee_data_map[name] = info;
            
            return ptr;
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
