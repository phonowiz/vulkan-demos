//
//  node.hpp
//  vulkan-demos
//
//  Created by Rafael Sabino on 1/18/20.
//  Copyright © 2020 Rafael Sabino. All rights reserved.
//

#pragma once

#include "EASTL/fixed_vector.h"

#include "command_recorder.h"
#include "camera.h"
#include "texture_registry.h"
#include "material_store.h"

#include <iostream>

namespace  vk
{
    class command_recorder;
    
    template<uint32_t NUM_CHILDREN>
    class node : public object
    {
    public:
        
        using node_type = vk::node<NUM_CHILDREN>;
        using tex_registry_type = texture_registry<NUM_CHILDREN>;
        using material_store_type = material_store;
        
        node(){}
        
        node(device* device)
        {
            _device = device;
        }
        
        
        static constexpr size_t MAX_COMMANDS = 20;
        
        inline void set_device(device* device)
        {
            _device = device;
        }
        node & operator=(const node&) = delete;
        node(const node&) = delete;
        node & operator=(node&) = delete;
        node(node&) = delete;
    
        void set_stores(texture_registry<NUM_CHILDREN>& tex_registry , material_store& mat_store)
        {
            _material_store = &mat_store;
            _texture_registry = &tex_registry;
        }
        
        //init shader parameters here
        virtual void init()
        {
            assert( _device != nullptr);

            debug_print("initting...");
            
            for( eastl_size_t i = 0; i < _children.size(); ++i)
            {
                _children[i]->set_stores(*_texture_registry, *_material_store);
                //std::cout << "\t\tinitting child of " << _name.c_str() << std::endl;
                _children[i]->init();
            }
            

            init_node();
            create_gpu_resources();
        }
        
        //?????
        //virtual void validate() = 0;
        
        //update shader parameters here every frame
        virtual void update(vk::camera& camera, uint32_t image_id) = 0;
        
        virtual void init_node() = 0;
        virtual void record_node_commands(command_recorder& buffer, uint32_t image_id) = 0;
        
        
        //use this to show on screen  a texture
        virtual bool debug_display(){ return false; };
        
        
        inline void set_enable(bool b){ _enable = b; }
        
        inline void add_child( node_type& child )
        {
            _children.push_back(&child);
            assert(_children.size() != 0 );
        }
        
        inline bool is_leaf(){
            return _children.empty();
        }
        
        node_type* get_child(uint32_t i)
        {
            assert(i < NUM_CHILDREN);
            return node_type::_children[i];
        }
        
        void reset()
        {
            _visited = false;
            for( int i = 0; i < _children.size(); ++i)
            {
                node_type::_children[i]->reset();
            }
            
        }
        
        virtual void record(command_recorder& buffer, uint32_t image_id)
        {
            _visited = false;
            for( int i = 0; i < _children.size(); ++i)
            {
                node_type::_children[i]->record(buffer, image_id);
            }
            
            std::cout << "recording node: " << _name.c_str() << std::endl;
            
            record_barriers(buffer, image_id);
            record_node_commands(buffer, image_id);
        }
        
        void set_name(const char* name)
        {
            _name = name;
        }
        
    protected:
        
        virtual void set_levels( uint32_t i )
        {
            assert(!_visited && "we have a cyclic dependency, revise your graph");
            _visited = true;
            
            _level = i;
            for( int i = 0; i < _children.size(); ++i)
            {
                node_type::_children[i]->set_levels(_level + 1);
            }
        }
        
        void debug_print(const char* message)
        {
            for( int i = 0; i <= _level; ++i)
            {
                std::cout << "\t";
            }
            std::cout << _name.c_str() << ": " << message << std::endl;
            
        }
        
        virtual void create_gpu_resources() = 0;
        
        void create_barrier(command_recorder& buffer, vk::image* p_image, typename tex_registry_type::node_dependees::iterator& b, uint32_t image_id)
        {
            VkImageMemoryBarrier barrier {};
            
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.pNext = nullptr;
            //TODO: this could potentially be set when connections are formed between nodes
            barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            barrier.oldLayout = static_cast<VkImageLayout>(p_image->get_native_layout());;
            barrier.newLayout = static_cast<VkImageLayout>((*b).layout);
            barrier.image = p_image->get_image();
            barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
            //we are not transferring ownership
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

            constexpr uint32_t VK_FLAGS_NONE = 0;

            vkCmdPipelineBarrier(
                                 buffer.get_raw_graphics_command(image_id),
                                 VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                 VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
                                 VK_FLAGS_NONE,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrier);
            
        }
        void record_barriers(command_recorder& buffer,  uint32_t image_id)
        {
            assert( _device->_queue_family_indices.graphics_family.value() ==
                   _device->_queue_family_indices.compute_family.value() && "If this assert fails, we will need to transfer dependent "
                                                                            "resources from compute to graphics queues and vice versa");
            
            using tex_registry_type = texture_registry<NUM_CHILDREN>;

            //note: here we are only grabbing those image resources this node depends on
            //in the graph, there may be more resources that are written to below this node, it doesn't mean this node depends on them
            typename tex_registry_type::node_dependees& dependees = _texture_registry->get_dependees(this);

            typename tex_registry_type::node_dependees::iterator begin = dependees.begin();
            typename tex_registry_type::node_dependees::iterator end = dependees.end();

            for(typename tex_registry_type::node_dependees::iterator b = begin ; b != end ; ++b)
            {
                
                std::shared_ptr<vk::object> res = std::static_pointer_cast<vk::object>((*b).data.resource);
                
                if(res->get_instance_type() == texture_2d::get_class_type() ||
                   res->get_instance_type() == texture_3d::get_class_type())
                {
                    std::shared_ptr<vk::image> p_image = std::static_pointer_cast<vk::image>(res);

                    create_barrier(buffer, p_image.get(), b, image_id );
//                    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
//                    barrier.pNext = nullptr;
//                    //TODO: this could potentially be set when connections are formed between nodes
//                    barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
//                    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
//
//                    barrier.oldLayout = static_cast<VkImageLayout>(p_image->get_native_layout());;
//                    barrier.newLayout = static_cast<VkImageLayout>((*b).layout);
//                    barrier.image = p_image->get_image();
//                    barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
//                    //we are not transferring ownership
//                    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
//                    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
//
//                    constexpr uint32_t VK_FLAGS_NONE = 0;
//
//                    vkCmdPipelineBarrier(
//                                         buffer.get_raw_graphics_command(image_id),
//                                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
//                                         VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
//                                         VK_FLAGS_NONE,
//                                         0, nullptr,
//                                         0, nullptr,
//                                         1, &barrier);
                }
                else
                {
                    //this is a resource set...
//                    assert(res->get_instance_type()  == resource_set<vk::texture_2d>::get_class_type() ||
//                           res->get_instance_type()  == resource_set<vk::depth_texture>::get_class_type() ||
//                           res->get_instance_type()  == resource_set<vk::texture_3d>::get_class_type() ||
//                           res->get_instance_type()  == resource_set<vk::render_texture>::get_class_type());
                    
                    if(res->get_instance_type()  == resource_set<vk::texture_2d>::get_class_type())
                    {
                        std::shared_ptr< resource_set<vk::texture_2d> > set = std::static_pointer_cast< resource_set<vk::texture_2d>>(res);
                        vk::texture_2d* tex = &((*set)[image_id]);
                        
                        create_barrier(buffer, tex, b, image_id );
                    }
                    if(res->get_instance_type()  == resource_set<vk::depth_texture>::get_class_type())
                    {
                        std::shared_ptr< resource_set<vk::depth_texture> > set = std::static_pointer_cast< resource_set<vk::depth_texture>>(res);
                        vk::image* tex = &((*set)[image_id]);
                        
                        create_barrier(buffer, tex, b, image_id );
                    }
                    if(res->get_instance_type()  == resource_set<vk::depth_texture>::get_class_type())
                    {
                        std::shared_ptr< resource_set<vk::texture_3d> > set = std::static_pointer_cast< resource_set<vk::texture_3d>>(res);
                        vk::image* tex = &((*set)[image_id]);
                        
                        create_barrier(buffer, tex, b, image_id );
                    }
                    if(res->get_instance_type()  == resource_set<vk::render_texture>::get_class_type())
                    {
                        std::shared_ptr< resource_set<vk::render_texture> > set = std::static_pointer_cast< resource_set<vk::render_texture>>(res);
                        vk::image* tex = &((*set)[image_id]);
                        
                        create_barrier(buffer, tex, b, image_id );
                    }
                    else
                    {
                        assert(0 && "unrecognized resource_set");
                    }
                    
                }
            }
            
        }
        
        
        
    protected:
        device* _device = nullptr;
        eastl::fixed_vector<node_type*, NUM_CHILDREN, true> _children{};
        
        material_store_type*     _material_store = nullptr;
        tex_registry_type*  _texture_registry = nullptr;
        
        eastl::fixed_string<char, 100> _name = "default";
        
        bool _active = false;
        bool _visited = false;
        bool _enable = true;
        
        uint32_t _level = 0;
        
    };
}
