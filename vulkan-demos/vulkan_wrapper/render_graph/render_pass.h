//
//  render_pass.hpp
//  vulkan-demos
//
//  Created by Rafael Sabino on 12/28/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#pragma once

#include <vector>
#include "EASTL/array.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_SILENT_WARNINGS

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include <limits>
#include <algorithm>

#include <glm/glm.hpp>
#include "depth_texture.h"
#include "texture_cube.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include "../core/glfw_swapchain.h"
#include "../core/object.h"

#include "../shapes/obj_shape.h"
#include "depth_texture.h"
#include "../core/device.h"
#include "graphics_pipeline.h"
#include "material_store.h"
#include "attachment_group.h"
#include "obj_shape.h"

namespace vk
{
    
    //TODO: You might need another argument for number of subpasses...
    template< uint32_t NUM_ATTACHMENTS>
    class render_pass : public object
    {
    public:
        using graphics_pipeline_type = graphics_pipeline< NUM_ATTACHMENTS>;
        using write_channels = typename graphics_pipeline_type::write_channels;
        
        static constexpr uint32_t MAX_NUMBER_OF_ATTACHMENTS = 50;
        static constexpr uint32_t MAX_SUBPASSES = 20u;
        static constexpr uint32_t MAX_OBJECTS = 50u;
        
        render_pass & operator=(const render_pass&) = delete;
        render_pass(const render_pass&) = delete;
        render_pass & operator=(render_pass&) = delete;
        render_pass(render_pass&) = delete;
        
        /////////////////////////////////////////////////////////////////////////////////////
        class subpass_s : object
        {
        public:
            
            //note: subpasses shouldn't be copied
            subpass_s & operator=(const subpass_s&) = delete;
            subpass_s(const subpass_s&) = delete;
            subpass_s & operator=(subpass_s&) = delete;
            subpass_s(subpass_s&) = delete;
            
            static constexpr int32_t INVALID_ATTACHMENT = -1;
            
            //TODO: We don't need this function, let's get rid of it. 
            inline bool is_active(){ return _active; }
            
            subpass_s()
            {
                for( int a = 0; a < MAX_NUMBER_OF_ATTACHMENTS; ++a)
                {
                    _color_references[a].attachment = eastl::numeric_limits<uint32_t>::max();
                    _color_references[a].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                    
                    _input_references[a].attachment = eastl::numeric_limits<uint32_t>::max();
                    _input_references[a].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    
                    _resolve_references[a].attachment = eastl::numeric_limits<uint32_t>::max();
                    _resolve_references[a].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                }
                //all objects affected by this subpass by defualt
                ignore_all_objs(false);
            }
            
            inline void ignore_object( uint32_t obj, bool b)
            {
                _subass_ignore[obj] = b;
            }
            
            inline void ignore_all_objs(bool b)
            {
                eastl::fill(_subass_ignore.begin(), _subass_ignore.end(), b);
            }
             
            inline void ignore_object(uint32_t obj_id)
            {
                EA_ASSERT(obj_id < _subass_ignore.size() );
                _subass_ignore[obj_id] = true;
            }
            
            inline bool is_ignored(uint32_t obj_id)
            {
                EA_ASSERT(obj_id < _subass_ignore.size() );
                return _subass_ignore[obj_id];
            }
            
            inline void set_cull_mode(typename graphics_pipeline_type::cull_mode mode)
            {
                for( int chain_id = 0; chain_id < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++chain_id)
                {
                    _pipeline[chain_id].set_cull_mode(mode);
                }
            }
            
            inline void set_device(device* d)
            {
                _device  = d;
                for( int i = 0; i < _pipeline.size(); ++i)
                {
                    _pipeline[i].set_device(_device);
                }
            }
            
            inline void set_viewport(glm::vec2 dimensions)
            {
                for( int chain_id = 0; chain_id < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++chain_id)
                {
                    _pipeline[chain_id].set_viewport((uint32_t)dimensions.x, (uint32_t)dimensions.y);
                }
            }
            inline void init()
            {
                _subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
                _subpass_description.pColorAttachments = _color_references.data();
                _subpass_description.colorAttachmentCount = _num_color_references;
                _subpass_description.pInputAttachments = _input_references.data();
                _subpass_description.inputAttachmentCount = _num_input_references;
                _subpass_description.pResolveAttachments = _num_resolve_references != 0 ? _resolve_references.data() : nullptr;
                
                EA_ASSERT_FORMATTED((_num_resolve_references == 0 ||_num_color_references == _num_resolve_references),
                              ("number color references (%i) and resolve references (%i) must be the same",
                              _num_color_references, _num_resolve_references));
                
#if EA_DEBUG
                if(_num_resolve_references)
                {
                    //verify that all color attachments are multi sampled
                    for( int i = 0; i < _num_color_references; ++i)
                    {
                        EA_ASSERT_MSG((*_attachment_group)[ _color_references[i].attachment ].is_multisampling(),  ("this subpass uses multisampling, but one of the input attachments is not "
                                                                                   "setup for multisampling.  When creating the input texture, make sure to "
                                                                                   "use \"set_multisampling\" and pass true"));
                    }
                }
#endif
            }
            
            inline bool is_depth_an_input()
            {
                bool result = false;
                for( int i = 0; i < _num_color_references; ++i)
                {
                    result = result || (*_attachment_group)[ _color_references[i].attachment ].get_instance_type() ==
                    resource_set<depth_texture>::get_class_type();
                }
                
                return result;
            }
            
            inline void set_id(uint32_t index)
            {
                _id = index;
            }
            
            
            inline void add_resolve_attachment( const char* name)
            {
                bool found = false;
                for( int i = 0; i < _attachment_group->size(); ++i)
                {
                   eastl::fixed_string<char, 50> n = name;
                   if( (*_attachment_group)[i].get_name() == n)
                   {
                       add_resolve_attachment( i );
                       found = true;
                       break;
                   }
                }
                
                EA_ASSERT_FORMATTED(found, ("resolve attachment %s not found.  Your resource set must be added to attachment group before calling this function", name));
            }
            
            inline void add_resolve_attachment( int32_t i)
            {
                EA_ASSERT_MSG((*_attachment_group)[i].get_instance_type() !=
                resource_set<depth_texture*>::get_class_type(), "depth resolve attachments are not supported in this version of vulkan,"
                              "they are supported on vulkan v1.2");
                
                _resolve_references[_num_resolve_references++].attachment = i;
                (*_attachment_group).set_multisample_attachment(i, false);
            }
            
            inline void add_output_attachment( const char* name)
            {
                bool found = false;
                for( int i = 0; i < _attachment_group->size(); ++i)
                {
                    eastl::fixed_string<char, 50> n = name;
                    if( (*_attachment_group)[i].get_name() == n)
                    {
                        if( !( (*_attachment_group)[i].get_instance_type() == resource_set<depth_texture*>::get_class_type() ||
                             (*_attachment_group)[i].get_instance_type() == resource_set<depth_texture>::get_class_type()) )
                        {
                            set_number_of_blend_attachments(++_blend_attachments);
                        }
                        
                        add_output_attachment( i );
                        found = true;
                        break;
                    }
                }

                EA_ASSERT_FORMATTED(found, ("Attachment %s not found.  Your resource set must be added to attachment group before calling this function", name));
            }
            
            inline void add_output_attachment( const char* name, write_channels wc, bool enable_blend)
            {
                
                bool found = false;
                for( int i = 0; i < _attachment_group->size(); ++i)
                {
                    eastl::fixed_string<char, 50> n = name;
                    if( (*_attachment_group)[i].get_name() == n)
                    {
                        
                        add_output_attachment( i );
                        
                        if( !((*_attachment_group)[i].get_instance_type() == resource_set<depth_texture*>::get_class_type() ||
                             (*_attachment_group)[i].get_instance_type() == resource_set<depth_texture>::get_class_type()))
                        {
                            set_number_of_blend_attachments(++_blend_attachments);
                            modify_attachment_blend(_blend_attachments-1, wc, enable_blend);
                        }
                        
                        found = true;
                        break;
                    }
                }

                EA_ASSERT_FORMATTED(found, ("Attachment %s not found.  Your resource set must be added to attachment group before calling \"add_output_attachment\" function", name));
            }
            
            inline void add_input_attachment( const char* parameter_name, const char* attachment_name,
                                             parameter_stage parameter_stage, uint32_t binding)
            {
                EA_ASSERT(_attachment_group != nullptr);
                
                int32_t id = (*_attachment_group).get_attachment_id(attachment_name);
                EA_ASSERT(id < NUM_ATTACHMENTS);
                
                _input_references[_num_input_references].attachment = id;
                _input_references[_num_input_references].layout =
                            static_cast<VkImageLayout>((*_attachment_group)[id][0]->get_usage_layout(vk::usage_type::INPUT_ATTACHMENT));
                
                ++_num_input_references;
                
                for( int chain_id = 0; chain_id < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++chain_id)
                {
                    _pipeline[chain_id].add_input_attachment((*_attachment_group)[id][chain_id], parameter_name, id, parameter_stage, binding);
                }
            }
            //TODO: TEMPLATES???
            inline void set_image_sampler(texture_cube& texture, const char* parameter_name,
                                          parameter_stage parameter_stage, uint32_t binding)
            {
                for( int chain_id = 0; chain_id < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++chain_id)
                {
                    _pipeline[chain_id].set_image_sampler(&texture, parameter_name, parameter_stage, binding, vk::usage_type::COMBINED_IMAGE_SAMPLER);
                }
            }
            inline void set_image_sampler(texture_3d& texture, const char* parameter_name,
                                          parameter_stage parameter_stage, uint32_t binding)
            {
                for( int chain_id = 0; chain_id < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++chain_id)
                {
                    _pipeline[chain_id].set_image_sampler(&texture, parameter_name, parameter_stage, binding, vk::usage_type::COMBINED_IMAGE_SAMPLER);
                }
            }
            
            inline void set_image_sampler(texture_2d& texture, const char* parameter_name,
                                          parameter_stage parameter_stage,  uint32_t binding)
            {
                for( int chain_id = 0; chain_id < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++chain_id)
                {
                    _pipeline[chain_id].set_image_sampler(texture, parameter_name, parameter_stage, binding, vk::usage_type::COMBINED_IMAGE_SAMPLER);
                }
            }
            
            inline void init_parameter(const char* parameter_name, parameter_stage stage,  float value, int32_t binding)
            {
                for( int chain_id = 0; chain_id < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++chain_id)
                {
                    _pipeline[chain_id].init_parameter(parameter_name, stage, value, binding);
                }
            };
            
            inline void init_parameter(const char* parameter_name, parameter_stage stage,  int32_t value, int32_t binding)
            {
                for( int chain_id = 0; chain_id < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++chain_id)
                {
                    _pipeline[chain_id].init_parameter(parameter_name, stage, value, binding);
                }
            };
            
            inline void init_parameter(const char* parameter_name, parameter_stage stage,  uint32_t value, int32_t binding)
            {
                for( int chain_id = 0; chain_id < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++chain_id)
                {
                    _pipeline[chain_id].init_parameter(parameter_name, stage, value, binding);
                }
            };
            
            inline void init_parameter(const char* parameter_name, parameter_stage stage,  glm::vec3 value, int32_t binding)
            {
                for( int chain_id = 0; chain_id < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++chain_id)
                {
                    _pipeline[chain_id].init_parameter(parameter_name, stage, value, binding);
                }
            };
            inline void init_parameter(const char* parameter_name, parameter_stage stage,  glm::vec4 value, int32_t binding)
            {
                for( int chain_id = 0; chain_id < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++chain_id)
                {
                    _pipeline[chain_id].init_parameter(parameter_name, stage, value, binding);
                }
            };
            inline void init_parameter(const char* parameter_name, parameter_stage stage,  glm::vec2 value, int32_t binding)
            {
                for( int chain_id = 0; chain_id < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++chain_id)
                {
                    _pipeline[chain_id].init_parameter(parameter_name, stage, value, binding);
                }
            }
            inline void init_parameter(const char* parameter_name, parameter_stage stage, glm::mat4 value, int32_t binding)
            {
                for( int chain_id = 0; chain_id < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++chain_id)
                {
                    _pipeline[chain_id].init_parameter(parameter_name, stage, value, binding);
                }
            }
            
            inline void init_parameter(const char* parameter_name, parameter_stage stage,
                                       glm::vec4* vecs,  size_t num_vectors, int32_t binding)
            {
                for( int chain_id = 0; chain_id < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++chain_id)
                {
                    _pipeline[chain_id].init_parameter(parameter_name, stage, vecs, num_vectors, binding);
                }
            }
            
            template<int MAX_SIZE>
            inline void init_parameter(const char* parameter_name, parameter_stage stage,
                                       eastl::array<int32_t,MAX_SIZE>& arr, int32_t binding)
            {
                for( int chain_id = 0; chain_id < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++chain_id)
                {
                    _pipeline[chain_id].template init_parameter<MAX_SIZE>(parameter_name, stage, arr, binding);
                }
            }
            
            template<int MAX_SIZE>
            inline void init_parameter(const char* parameter_name, parameter_stage stage,
                                       eastl::array<glm::vec4,MAX_SIZE>& arr, int32_t binding)
            {
                for( int chain_id = 0; chain_id < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++chain_id)
                {
                    _pipeline[chain_id].template init_parameter<MAX_SIZE>(parameter_name, stage, arr, binding);
                }
            }
            
            inline void init_dynamic_params(const char* parameter_name, parameter_stage stage,
                                            glm::mat4& val, size_t num_objs, int32_t binding)
            {
                
                for( int chain_id = 0; chain_id < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++chain_id)
                {
                    for( int j = 0; j < num_objs; ++j)
                        _pipeline[chain_id].get_dynamic_parameters(stage, binding)[j][parameter_name] = val;
                }
            }
            
            inline void set_image_sampler(resource_set<texture_3d>& textures, const char* parameter_name,
                                          parameter_stage parameter_stage, uint32_t binding)
            {
                for( int chain_id = 0; chain_id < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++chain_id)
                {
                    _pipeline[chain_id].set_image_sampler(textures[chain_id], parameter_name, parameter_stage, binding, textures.get_last_transition().current_usage_type);
                }
            }
            
            inline void set_image_sampler(resource_set<texture_2d>& textures, const char* parameter_name,
                                          parameter_stage parameter_stage, uint32_t binding)
            {
                for( int chain_id = 0; chain_id < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++chain_id)
                {
                    _pipeline[chain_id].set_image_sampler(textures[chain_id], parameter_name, parameter_stage, binding, textures.get_last_transition().current_usage_type) ;
                }
            }
            
            inline void set_image_sampler(resource_set<render_texture>& textures, const char* parameter_name,
                                          parameter_stage parameter_stage, uint32_t binding)
            {
                for( int chain_id = 0; chain_id < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++chain_id)
                {
                    _pipeline[chain_id].set_image_sampler(textures[chain_id], parameter_name, parameter_stage, binding, textures.get_last_transition().current_usage_type) ;
                }
            }
            
            
            inline void set_image_sampler(resource_set<texture_cube>& textures, const char* parameter_name,
                                          parameter_stage parameter_stage, uint32_t binding)
            {
                for( int chain_id = 0; chain_id < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++chain_id)
                {
                    _pipeline[chain_id].set_image_sampler(textures[chain_id], parameter_name, parameter_stage, binding, textures.get_last_transition().current_usage_type) ;
                }
            }
            
            inline void set_image_sampler(resource_set<depth_texture>& textures, const char* parameter_name,
                                          parameter_stage parameter_stage, uint32_t binding)
            {
                for( int chain_id = 0; chain_id < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++chain_id)
                {
                    _pipeline[chain_id].set_image_sampler(textures[chain_id], parameter_name, parameter_stage, binding, textures.get_last_transition().current_usage_type) ;
                }
            }
            
            inline void set_polygon_fill(polygon_mode mode)
            {
                for( int chain_id = 0; chain_id < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++chain_id)
                {
                    _pipeline[chain_id].set_polygon_fill(mode) ;
                }
            }
            
            
            void set_material( material_store& store, const char* material_name)
            {
                for( int chain_id = 0; chain_id < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++chain_id)
                {
                    _pipeline[chain_id].set_material(store.GET_MAT<visual_material>(material_name));
                }
            }
            
            inline VkSubpassDescription get_subpass_description() { return _subpass_description; }
            
            inline void commit_parameters_to_gpu(uint32_t swapchain_id)
            {
                _pipeline[swapchain_id].commit_parameters_to_gpu();
            }
            
            inline graphics_pipeline_type& get_pipeline(uint32_t swapchain_id ){ return _pipeline[swapchain_id]; }
            
            inline void create(VkRenderPass& vk_render_pass, uint32_t swapchain_id)
            {
                _pipeline[swapchain_id].set_multisampling(_attachment_group->is_multisampling());
                _pipeline[swapchain_id].create(vk_render_pass, _id);
            }
            
            inline void set_attachment_group( attachment_group<NUM_ATTACHMENTS>* group)
            {
                _attachment_group = group;
            }
            
            inline void destroy()
            {
                for( int i = 0; i < _pipeline.size(); ++i)
                {
                    _pipeline[i].destroy();
                }
            }

            inline void begin_subpass_recording(VkCommandBuffer& buffer, uint32_t swapchain_image_id, uint32_t object_id)
            {
                vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,_pipeline[swapchain_image_id].get_vk_pipeline());
                
                _pipeline[swapchain_image_id].bind_material_assets( buffer, object_id);
            }
            
            inline bool get_depth_enable( ) { return _depth_enable; }
            inline void set_active(){ _active = true; }
            inline bool multisampling_enalbed(){ return _num_resolve_references !=0; }
            
            eastl::fixed_string<char, 100> _name = {};
        private:

            inline void set_number_of_blend_attachments( uint32_t blend_attachments)
            {
                for( int chain_id = 0; chain_id < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++chain_id)
                {
                    _pipeline[chain_id].set_number_of_blend_attachments(blend_attachments);
                }
            }
            
            void modify_attachment_blend( uint32_t index, write_channels channels, bool depth_enable)
            {
                for( int chain_id = 0; chain_id < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++chain_id)
                {
                    _pipeline[chain_id].modify_attachment_blend(index, channels, depth_enable);
                }
            }
            
            inline void set_depth_enable( bool depth) { _depth_enable = depth; }
            
            inline void add_output_attachment( uint32_t attachment_id)
            {
                if( (*_attachment_group)[attachment_id].get_instance_type() ==
                        resource_set<depth_texture*>::get_class_type())
                {
                    set_depth_enable(true);
                }
                else
                    _color_references[_num_color_references++].attachment = attachment_id;
            }
            
            VkSubpassDescription _subpass_description {};
            bool _active = false;
            uint32_t _id = INVALID_ATTACHMENT;
            uint32_t _num_color_references = 0;
            uint32_t _num_input_references = 0;
            uint32_t _num_resolve_references = 0;
            uint32_t _blend_attachments = 0;

            eastl::array<VkAttachmentReference, MAX_NUMBER_OF_ATTACHMENTS> _color_references {};
            eastl::array<VkAttachmentReference, MAX_NUMBER_OF_ATTACHMENTS> _input_references {};
            eastl::array<VkAttachmentReference, MAX_NUMBER_OF_ATTACHMENTS> _resolve_references {};
            
            eastl::array<graphics_pipeline_type, glfw_swapchain::NUM_SWAPCHAIN_IMAGES> _pipeline;
            eastl::array< bool, MAX_OBJECTS> _subass_ignore {};
            
            attachment_group<NUM_ATTACHMENTS>* _attachment_group = nullptr;
            device* _device = nullptr;
            bool _depth_enable = false;
        };
        ///////////////////////////////////////////////////////////////////////////////////// subpass
        
        
        inline uint32_t get_num_objs()
        {
            return _num_objects;
        }
        
        inline void add_object( obj_shape* obj)
        {
            _shapes[_num_objects] = obj;
            _num_objects++;
        }
        
        inline obj_shape* get_object(uint32_t obj_id)
        {
            EA_ASSERT(_shapes.size() > obj_id);
            return _shapes[obj_id];
        }
        
        inline void commit_parameters_to_gpu(uint32_t swapchain_id)
        {
            for( int subpass_id = 0; subpass_id < _subpasses.size(); ++subpass_id)
            {
                if(!_subpasses[subpass_id].is_active()) break;
                //TODO: make _vk_render_passes of size 1
                _subpasses[subpass_id].commit_parameters_to_gpu(swapchain_id);
            }
        }
        
        inline void next_subpass(VkCommandBuffer& buffer)
        {
            vkCmdNextSubpass(buffer, VK_SUBPASS_CONTENTS_INLINE);
        }
        attachment_group<NUM_ATTACHMENTS>& get_attachment_group();

        render_pass(){ }
        render_pass(device* device, glm::vec2 dimensions);
        
        inline bool is_depth_enabled()
        {
            bool result = false;
            int i = 0;
            while(_subpasses[i].is_active() && !result)
                result = _subpasses[i++].get_depth_enable();
            
            return result;
        }
        
        void init(uint32_t swapchain_id);
        
        void record_draw_commands(VkCommandBuffer& buffer, uint32_t swapchain_id, uint32_t instance_count);
        
        inline VkRenderPass& get_vk_render_pass(uint32_t i)
        {
            EA_ASSERT( i < _vk_render_passes.size());
            return _vk_render_passes[i];

        };
        
        inline VkImageView& get_vk_depth_image_view(uint32_t i)
        {
            EA_ASSERT(_attachment_group->get_depth_set() != nullptr);
            
            resource_set<depth_texture>& depth = *(_attachment_group->get_depth_set());
            
            EA_ASSERT( i < depth.size());
            return depth[i]._image_view;
            
        }
        
        inline VkFramebuffer& get_vk_frame_buffer( uint32_t swapchain_id)
        {
            EA_ASSERT(_vk_frame_buffer_infos.size() > swapchain_id);
            return _vk_frame_buffer_infos[swapchain_id];
        }
        
        inline resource_set<image*>& get_depth_textures()
        {
            EA_ASSERT_MSG(_attachment_group.get_depth_set() != nullptr, "This render pass doesn't have a depth texture as an attachment");
            
            resource_set<image*>& depth = *(_attachment_group.get_depth_set());
            return depth;
        }
        
        inline void set_device(device* device)
        {
            _device = device;
            _attachment_group.set_device(device);
        }
        
        
        inline graphics_pipeline_type& get_pipeline( uint32_t swapchain_id,  uint32_t subpass_id){ return get_subpass(subpass_id).get_pipeline(swapchain_id);}

        inline subpass_s& add_subpass(vk::material_store* store,  const char* material_name, const char* subpass_name = "" )
        {
            return add_subpass(*store, material_name, subpass_name);
        }
        
        inline subpass_s& add_subpass(vk::material_store& store,  const char* material_name, const char* subpass_name = "" )
        {
            _subpasses[_num_subpasses]._name = subpass_name;
            _subpasses[_num_subpasses].set_active();
            _subpasses[_num_subpasses].set_device(_device);
            _subpasses[_num_subpasses].set_material(store, material_name);
            _subpasses[_num_subpasses].set_attachment_group(&_attachment_group);
            _subpasses[_num_subpasses].set_id(_num_subpasses);

            return _subpasses[_num_subpasses++];
        };
        
        inline uint32_t get_number_of_subpasses()
        {
            return _num_subpasses;
        }
        
        inline subpass_s& get_subpass(uint32_t subpass_id)
        {
            EA_ASSERT_MSG(subpass_id < _num_subpasses,"you asking for subpass that doesn't exist, call \"add_subpass\" on the render_pass object first");
            return _subpasses[subpass_id];
        }
        
        inline void init_attachment_group()
        {
            _attachment_group.init();
        }
        inline void create(uint32_t swapchain_id)
        {
            EA_ASSERT_MSG(_num_subpasses != 0, "you need at least one subpass");
            init(swapchain_id);
            for( int subpass_id = 0; subpass_id < _num_subpasses; ++subpass_id)
            {
                if(!_subpasses[subpass_id].is_active()) break;
                //TODO: make _vk_render_passes of size 1.  It might be possible to just have one, frame buffers however, you'll need one per swapchain image
                _subpasses[subpass_id].create(_vk_render_passes[swapchain_id], swapchain_id);
            }
        }
        
        inline void set_dimensions(glm::vec2 dims)
        {
            _dimensions = dims;
            _attachment_group.set_dimensions(dims);
        }
        
        inline glm::vec2 get_dimensions()
        {
            return _dimensions;
        }
        
        void destroy() override;
        
    private:
        
        void create_frame_buffers(uint32_t swapchain_id);
        void begin_render_pass(VkCommandBuffer& buffer, uint32_t swapchain_image_id);
        void end_render_pass(VkCommandBuffer& buffer);

        
    private:
        
        attachment_group<NUM_ATTACHMENTS>  _attachment_group;
        eastl::array<VkRenderPass,glfw_swapchain::NUM_SWAPCHAIN_IMAGES>   _vk_render_passes {};
        eastl::array<VkFramebuffer, glfw_swapchain::NUM_SWAPCHAIN_IMAGES> _vk_frame_buffer_infos {};
        
        eastl::array<subpass_s, MAX_SUBPASSES> _subpasses {};
        eastl::array<obj_shape*, MAX_OBJECTS> _shapes {};
        
        static_assert(MAX_NUMBER_OF_ATTACHMENTS > NUM_ATTACHMENTS, "Number of attachments in your render pass excees what we can handle, increase limit??");

        glm::vec2 _dimensions {};
        device* _device = nullptr;
        uint32_t _num_subpasses = 0;
        uint32_t _num_objects = 0;
    };

#include "render_pass.hpp"
}
