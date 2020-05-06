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
        
        static constexpr uint32_t MAX_NUMBER_OF_ATTACHMENTS = 10u;
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
            
            inline bool is_active(){ return _active; }
            
            enum class layout
            {
                COLOR_ATTACHMENT = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                SHADER_READ_ONLY = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                PRESENT_LAYOUT = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
            };
            
            subpass_s()
            {
                for( int a = 0; a < MAX_NUMBER_OF_ATTACHMENTS; ++a)
                {
                    _color_references[a].attachment = std::numeric_limits<uint32_t>::max();
                    _color_references[a].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                    
                    _input_references[a].attachment = std::numeric_limits<uint32_t>::max();
                    _input_references[a].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                }
                //all objects affected by this subpass by defualt
                std::fill(_subpass_activate.begin(), _subpass_activate.end(), false);
            }
             
            inline void ignore_object(uint32_t obj_id)
            {
                assert(obj_id < _subpass_activate.size() );
                _subpass_activate[obj_id] = true;
            }
            
            inline bool is_ignored(uint32_t obj_id)
            {
                assert(obj_id < _subpass_activate.size() );

                return _subpass_activate[obj_id];
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
                assert((_num_input_references != 0 ||  _num_color_references != 0 ) && "subpassses must have at least one input or out attachment");
                _subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
                _subpass_description.pColorAttachments = _color_references.data();
                _subpass_description.colorAttachmentCount = _num_color_references;
                _subpass_description.pInputAttachments = _input_references.data();
                _subpass_description.inputAttachmentCount = _num_input_references;
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

                assert(found && "Attachment name not found.  Your resource set must be added to attachment group before calling this function");
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

                assert(found && "Attachment name not found.  Your resource set must be added to attachment group before calling this function");
            }
            
            inline void add_input_attachment( const char* parameter_name, const char* attachment_name,
                                             visual_material::parameter_stage parameter_stage, uint32_t binding)
            {
                assert(_attachment_group != nullptr);
                
                int32_t id = (*_attachment_group).get_attachment_id(attachment_name);
                assert(id < NUM_ATTACHMENTS);
                
                _input_references[_num_input_references].attachment = id;
                _input_references[_num_input_references].layout =
                            static_cast<VkImageLayout>((*_attachment_group)[id][0]->get_usage_layout(resource::usage_type::INPUT_ATTACHMENT));
                
                ++_num_input_references;
                
                for( int chain_id = 0; chain_id < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++chain_id)
                {
                    _pipeline[chain_id].add_input_attachment((*_attachment_group)[id][chain_id], parameter_name, id, parameter_stage, binding);
                }
            }
            
            inline void set_image_sampler(texture_3d& texture, const char* parameter_name,
                                          visual_material::parameter_stage parameter_stage, uint32_t binding,  resource::usage_type usage)
            {
                for( int chain_id = 0; chain_id < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++chain_id)
                {
                    _pipeline[chain_id].set_image_sampler(&texture, parameter_name, parameter_stage, binding, usage);
                }
            }
            
            inline void set_image_sampler(texture_2d& texture, const char* parameter_name,
                                          visual_material::parameter_stage parameter_stage,  uint32_t binding, resource::usage_type usage)
            {
                for( int chain_id = 0; chain_id < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++chain_id)
                {
                    _pipeline[chain_id].set_image_sampler(texture, parameter_name, parameter_stage, binding, usage);
                }
            }
            inline void set_image_sampler(eastl::array<depth_texture, glfw_swapchain::NUM_SWAPCHAIN_IMAGES>& textures, const char* parameter_name,
                                          visual_material::parameter_stage parameter_stage, uint32_t binding,  resource::usage_type usage)
            {
                for( int chain_id = 0; chain_id < textures.size(); ++chain_id)
                {
                    _pipeline[chain_id].set_image_sampler(textures[chain_id], parameter_name, parameter_stage, binding, usage) ;
                }
            }
            
            inline void set_image_sampler(eastl::array<render_texture, glfw_swapchain::NUM_SWAPCHAIN_IMAGES>& textures, const char* parameter_name,
                                          visual_material::parameter_stage parameter_stage, uint32_t binding, resource::usage_type usage)
            {
                for( int chain_id = 0; chain_id < textures.size(); ++chain_id)
                {
                    _pipeline[chain_id].set_image_sampler(textures[chain_id], parameter_name, parameter_stage, binding, usage) ;
                }
            }
            
            inline void init_parameter(const char* parameter_name, visual_material::parameter_stage stage,  float value, int32_t binding)
            {
                for( int chain_id = 0; chain_id < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++chain_id)
                {
                    _pipeline[chain_id].init_parameter(parameter_name, stage, value, binding);
                }
            };
            
            inline void init_parameter(const char* parameter_name, visual_material::parameter_stage stage,  int32_t value, int32_t binding)
            {
                for( int chain_id = 0; chain_id < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++chain_id)
                {
                    _pipeline[chain_id].init_parameter(parameter_name, stage, value, binding);
                }
            };
            
            inline void init_parameter(const char* parameter_name, visual_material::parameter_stage stage,  uint32_t value, int32_t binding)
            {
                for( int chain_id = 0; chain_id < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++chain_id)
                {
                    _pipeline[chain_id].init_parameter(parameter_name, stage, value, binding);
                }
            };
            
            inline void init_parameter(const char* parameter_name, visual_material::parameter_stage stage,  glm::vec3 value, int32_t binding)
            {
                for( int chain_id = 0; chain_id < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++chain_id)
                {
                    _pipeline[chain_id].init_parameter(parameter_name, stage, value, binding);
                }
            };
            inline void init_parameter(const char* parameter_name, visual_material::parameter_stage stage,  glm::vec4 value, int32_t binding)
            {
                for( int chain_id = 0; chain_id < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++chain_id)
                {
                    _pipeline[chain_id].init_parameter(parameter_name, stage, value, binding);
                }
            };
            inline void init_parameter(const char* parameter_name, visual_material::parameter_stage stage,  glm::vec2 value, int32_t binding)
            {
                for( int chain_id = 0; chain_id < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++chain_id)
                {
                    _pipeline[chain_id].init_parameter(parameter_name, stage, value, binding);
                }
            }
            inline void init_parameter(const char* parameter_name, visual_material::parameter_stage stage, glm::mat4 value, int32_t binding)
            {
                for( int chain_id = 0; chain_id < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++chain_id)
                {
                    _pipeline[chain_id].init_parameter(parameter_name, stage, value, binding);
                }
            }
            
            inline void init_parameter(const char* parameter_name, visual_material::parameter_stage stage,
                                       glm::vec4* vecs,  size_t num_vectors, int32_t binding)
            {
                for( int chain_id = 0; chain_id < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++chain_id)
                {
                    _pipeline[chain_id].init_parameter(parameter_name, stage, vecs, num_vectors, binding);
                }
            }
            
            inline void init_dynamic_params(const char* parameter_name, visual_material::parameter_stage stage,
                                            glm::mat4& val, size_t num_objs, int32_t binding)
            {
                
                for( int chain_id = 0; chain_id < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++chain_id)
                {
                    for( int j = 0; j < num_objs; ++j)
                        _pipeline[chain_id].get_dynamic_parameters(stage, binding)[j][parameter_name] = val;
                }
            }
            
            inline void set_image_sampler(resource_set<texture_3d>& textures, const char* parameter_name,
                                          visual_material::parameter_stage parameter_stage, uint32_t binding, resource::usage_type usage)
            {
                for( int chain_id = 0; chain_id < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++chain_id)
                {
                    _pipeline[chain_id].set_image_sampler(textures[chain_id], parameter_name, parameter_stage, binding, usage) ;
                }
            }
            
            inline void set_image_sampler(resource_set<texture_2d>& textures, const char* parameter_name,
                                          visual_material::parameter_stage parameter_stage, uint32_t binding, resource::usage_type usage)
            {
                for( int chain_id = 0; chain_id < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++chain_id)
                {
                    _pipeline[chain_id].set_image_sampler(textures[chain_id], parameter_name, parameter_stage, binding, usage) ;
                }
            }
            
            inline void set_image_sampler(resource_set<depth_texture>& textures, const char* parameter_name,
                                          visual_material::parameter_stage parameter_stage, uint32_t binding, resource::usage_type usage)
            {
                for( int chain_id = 0; chain_id < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++chain_id)
                {
                    _pipeline[chain_id].set_image_sampler(textures[chain_id], parameter_name, parameter_stage, binding, usage) ;
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
            
            const char* _name = nullptr;
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
            uint32_t _blend_attachments = 0;

            eastl::array<VkAttachmentReference, MAX_NUMBER_OF_ATTACHMENTS> _color_references {};
            eastl::array<VkAttachmentReference, MAX_NUMBER_OF_ATTACHMENTS> _input_references {};
            eastl::array<graphics_pipeline_type, glfw_swapchain::NUM_SWAPCHAIN_IMAGES> _pipeline;
            eastl::array< bool, MAX_OBJECTS> _subpass_activate {};
            
            attachment_group<NUM_ATTACHMENTS>* _attachment_group = nullptr;
            device* _device = nullptr;
            bool _depth_enable = false;
        };
        /////////////////////////////////////////////////////////////////////////////////////
        
        
        inline void add_object( obj_shape& obj)
        {
            _shapes[_num_objects] = &obj;
            _num_objects++;
        }
        
        inline obj_shape* get_object(uint32_t obj_id)
        {
            assert(_shapes.size() > obj_id);
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
        
        inline bool skip_subpass(obj_shape& shape, uint32_t subpass_id)
        {
            bool result = false;
            for( uint32_t i = 0; i < _num_objects; ++i)
            {
                if(_shapes[i] == &shape)
                {
                    get_subpass(subpass_id).ignore_object(i);
                    result = true;
                }
            }
            assert( result == true && "you are asking to ignore an object not yet added to render pass");
            return result;
        }
        
        void init(uint32_t swapchain_id);
        
        void record_draw_commands(VkCommandBuffer& buffer, uint32_t swapchain_id);
        
        inline VkRenderPass& get_vk_render_pass(uint32_t i)
        {
            assert( i < _vk_render_passes.size());
            return _vk_render_passes[i];

        };
        
        inline VkImageView& get_vk_depth_image_view(uint32_t i)
        {
            assert(_attachment_group->get_depth_set() != nullptr);
            
            resource_set<depth_texture>& depth = *(_attachment_group->get_depth_set());
            
            assert( i < depth.size());
            return depth[i]._image_view;
            
        }
        
        inline VkFramebuffer& get_vk_frame_buffer( uint32_t swapchain_id)
        {
            assert(_vk_frame_buffer_infos.size() > swapchain_id);
            return _vk_frame_buffer_infos[swapchain_id];
        }
        
        inline resource_set<image*>& get_depth_textures()
        {
            assert(_attachment_group.get_depth_set() != nullptr && "This render pass doesn't have a depth texture as an attachment");
            
            resource_set<image*>& depth = *(_attachment_group.get_depth_set());
            return depth;
        }
        
        inline void set_device(device* device)
        {
            _device = device;
            _attachment_group.set_device(device);
        }
        
        
        inline graphics_pipeline_type& get_pipeline( uint32_t swapchain_id, uint32_t subpass_id){ return get_subpass(subpass_id).get_pipeline(swapchain_id);}

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
            uint32_t result = 0;
            
            for( int i = 0; i < _subpasses.size(); ++i)
            {
                result += _subpasses[i].is_active() ? 1 : 0;
            }
            
            return result;
        }
        
        inline subpass_s& get_subpass(uint32_t subpass_id)
        {
            assert(subpass_id < _num_subpasses && "you asking for subpass that doesn't exist, call \"add_subpass\" on the render_pass object first");
            return _subpasses[subpass_id];
        }
        
        inline void create(uint32_t swapchain_id)
        {
            assert(_num_subpasses != 0 && "you need at least one subpass");
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

    template<uint32_t NUM_ATTACHMENTS>
    void render_pass< NUM_ATTACHMENTS>::record_draw_commands(VkCommandBuffer& buffer, uint32_t swapchain_id)
    {
        assert(_num_objects != 0 && "you must have objects to render in a subpass");
        begin_render_pass(buffer, swapchain_id);

        
        for( uint32_t subpass_id = 0; subpass_id < _num_subpasses; ++subpass_id)
        {
            int drawn_obj = 0;
            for( uint32_t obj_id = 0; obj_id < _num_objects; ++obj_id)
            {
                if(!_subpasses[subpass_id].is_ignored(obj_id))
                {
                    _subpasses[subpass_id].begin_subpass_recording(buffer, swapchain_id, drawn_obj );
                    for( uint32_t mesh_id = 0; mesh_id < _shapes[obj_id]->get_num_meshes(); ++mesh_id)
                    {
                        _shapes[obj_id]->bind_verteces(buffer, mesh_id);
                        _shapes[obj_id]->draw_indexed(buffer, mesh_id );
                    }
                    ++drawn_obj;
                }
            }
            if(_num_subpasses != (subpass_id + 1))
                next_subpass(buffer);
        }
        
        end_render_pass(buffer);
    }

    template< uint32_t NUM_ATTACHMENTS>
    render_pass< NUM_ATTACHMENTS>::render_pass(vk::device* device,  glm::vec2 dimensions):
    _attachment_group(device, dimensions)
    {
        _dimensions = dimensions;
        _device = device;

        for( int subpass_id = 0; subpass_id < _subpasses.size(); ++subpass_id )
        {
            _subpasses[subpass_id].set_device(device);
            _subpasses[subpass_id].set_viewport(dimensions);
        }
    }

    template < uint32_t NUM_ATTACHMENTS>
    attachment_group<NUM_ATTACHMENTS>& render_pass< NUM_ATTACHMENTS>::get_attachment_group()
    {
        return _attachment_group;
    }

    template < uint32_t NUM_ATTACHMENTS>
    void render_pass< NUM_ATTACHMENTS>::init(uint32_t swapchain_id)
    {
        assert(_attachment_group.size() <= MAX_NUMBER_OF_ATTACHMENTS);
        assert(_attachment_group.size() != 0);
        assert(_dimensions.x != 0 && _dimensions.y !=0 );
        assert(_device != nullptr);
        
        _attachment_group.init(swapchain_id);

        eastl::array<VkAttachmentDescription, MAX_NUMBER_OF_ATTACHMENTS> attachment_descriptions {};
        //an excellent explanation of what the heck are these attachment references:
        //https://stackoverflow.com/questions/49652207/what-is-the-purpose-of-vkattachmentreference
        
        //here is article about subpasses and input attachments and how they are all tied togethere
        //https://www.saschawillems.de/blog/2018/07/19/vulkan-input-attachments-and-sub-passes/
        uint32_t attachment_id = 0;
        assert(_attachment_group[attachment_id].size() == glfw_swapchain::NUM_SWAPCHAIN_IMAGES);
        
        for(int i =0 ; i < _attachment_group.size(); ++i)
        {
            
            assert(_attachment_group.size() != 0);
            assert(_attachment_group[i][swapchain_id]->is_initialized() && "call init on all your attachment group textures");
            _attachment_group[i][swapchain_id]->set_device(_device);
            assert(_dimensions.x == _attachment_group[i][swapchain_id]->get_width());
            assert(_dimensions.y == _attachment_group[i][swapchain_id]->get_height());
            
            //depth will be dealt with later in this function...
            if(_attachment_group[i][0]->get_instance_type() == depth_texture::get_class_type())
                continue;
            
            attachment_descriptions[attachment_id].samples = VK_SAMPLE_COUNT_1_BIT;
            attachment_descriptions[attachment_id].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            //TODO: MAYBE THIS NEEDS TO BE SPECIFIED BY USER, YOU DON'T HAVE TO STORE ALL THE TIME...
            attachment_descriptions[attachment_id].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachment_descriptions[attachment_id].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachment_descriptions[attachment_id].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            
            attachment_descriptions[attachment_id].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachment_descriptions[attachment_id].finalLayout = static_cast<VkImageLayout>(_attachment_group[i][swapchain_id]->get_native_layout());
            attachment_descriptions[attachment_id].format = static_cast<VkFormat>(_attachment_group[i][swapchain_id]->get_format());
            
            attachment_id++;
            
        }
        
        assert(attachment_id != 0);
        
        eastl::array<VkSubpassDescription, MAX_SUBPASSES> subpass {};
        
        assert(_subpasses[0].is_active() && "You need at least one subpass for rendering to occur");
        int subpass_id = 0;
        
        eastl::array<VkSubpassDependency,MAX_SUBPASSES> dependencies {};
        
        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = 0;
        dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        
        VkAttachmentReference depth_reference {};
        
        while(_subpasses[subpass_id].is_active())
        {
            _subpasses[subpass_id].init();
            subpass[subpass_id] = _subpasses[subpass_id].get_subpass_description();
            
            //an excellent explanation of what the heck are these attachment references:
            //https://stackoverflow.com/questions/49652207/what-is-the-purpose-of-vkattachmentreference
            
             if(_subpasses[subpass_id].get_depth_enable())
            {
                assert( _subpasses[subpass_id].is_depth_an_input() != true && "depth cannot be both an input an output in subpass, call subpass.set_depth_enable");
                //note: in this code base, the last attachement is the depth, the frame buffer creating code assumes this as well.
                resource_set<image*>& depths =  get_depth_textures();
                depth_texture* t = static_cast<depth_texture*>( depths[swapchain_id]);
                attachment_descriptions[attachment_id] =  t->get_depth_attachment();
                depth_reference.attachment = attachment_id;
                depth_reference.layout = static_cast<VkImageLayout>(depths[swapchain_id]->get_usage_layout(resource::usage_type::STORAGE_IMAGE));
                subpass[subpass_id].pDepthStencilAttachment = &depth_reference;
                ++attachment_id;
            }

            //note: for a great explanation of VK_SUBPASS_EXTERNAL:
            //https://stackoverflow.com/questions/53984863/what-exactly-is-vk-subpass-external?rq=1
            
            uint32_t d = subpass_id + 1;
            bool last = (d ==  MAX_SUBPASSES || !_subpasses[d].is_active());
            dependencies[d].srcSubpass = subpass_id;
            dependencies[d].dstSubpass = last ? VK_SUBPASS_EXTERNAL : d;
            dependencies[d].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependencies[d].dstStageMask = last ? VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT : VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            dependencies[d].srcAccessMask = last ? VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT : VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            dependencies[d].dstAccessMask = last ? VK_ACCESS_MEMORY_READ_BIT : VK_ACCESS_SHADER_READ_BIT ;
            dependencies[d].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
            
            ++subpass_id;
        }

        VkRenderPassCreateInfo render_pass_info = {};
        render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        render_pass_info.pAttachments = attachment_descriptions.data();
        render_pass_info.attachmentCount =attachment_id;
        render_pass_info.subpassCount = subpass_id;
        render_pass_info.pSubpasses = subpass.data();
        render_pass_info.dependencyCount = subpass_id;
        render_pass_info.pDependencies = dependencies.data();

        VkResult result = vkCreateRenderPass(_device->_logical_device, &render_pass_info, nullptr, &_vk_render_passes[swapchain_id]);
        ASSERT_VULKAN(result);
        
        create_frame_buffers(swapchain_id);
    }

    template < uint32_t NUM_ATTACHMENTS>
    void render_pass< NUM_ATTACHMENTS>::create_frame_buffers(uint32_t swapchain_id)
    {
        eastl::array<VkImageView, MAX_NUMBER_OF_ATTACHMENTS> attachment_views {};
        
        assert(_attachment_group.size() < MAX_NUMBER_OF_ATTACHMENTS);
        uint32_t num_views = 0;
        
        //add all num views for this swapchain id
        for( int i = 0; i < _attachment_group.size(); ++i)
        {
            //depth will be dealt with later in this function...
            if(_attachment_group[i][0]->get_instance_type() == depth_texture::get_class_type())
                continue;
            assert(_attachment_group[i][swapchain_id]->is_initialized());
            assert(_attachment_group[i][swapchain_id] != nullptr);
            assert(_attachment_group[i][swapchain_id]->_image_view != VK_NULL_HANDLE && "did you initialize this image?");
            attachment_views[num_views++] = _attachment_group[i][swapchain_id]->_image_view;
        }
        
        if(is_depth_enabled())
        {
            resource_set<image*>& depths =  get_depth_textures();
            assert(depths[swapchain_id]->_image_view != VK_NULL_HANDLE);
            //the render pass assume this as well... depth texture is the last one...
            attachment_views[num_views++]  = depths[swapchain_id]->_image_view;
        }
        
        VkFramebufferCreateInfo framebuffer_create_info {};
        framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_create_info.pNext = nullptr;
        framebuffer_create_info.flags = 0;
        framebuffer_create_info.renderPass = _vk_render_passes[swapchain_id];
        framebuffer_create_info.attachmentCount = num_views;
        framebuffer_create_info.pAttachments = attachment_views.data();
        //there is an assumption that all attachments are the same width and height, I put some
        //asserts before which will check if this is true
        framebuffer_create_info.width = _dimensions.x;
        framebuffer_create_info.height = _dimensions.y;
        framebuffer_create_info.layers = 1;
        
        VkResult result = vkCreateFramebuffer(_device->_logical_device, &framebuffer_create_info, nullptr, &(_vk_frame_buffer_infos[swapchain_id]));
        ASSERT_VULKAN(result)
        
    }

    template < uint32_t NUM_ATTACHMENTS>
    void render_pass< NUM_ATTACHMENTS>::destroy()
    {
        
        for( int i = 0; i <  _subpasses.size(); ++i)
        {
            if(!_subpasses[i].is_active()) break;
            
            _subpasses[i].destroy();
        }
        for( int i =0 ; i < _vk_frame_buffer_infos.size(); ++i)
        {
            vkDestroyFramebuffer(_device->_logical_device, _vk_frame_buffer_infos[i], nullptr);
        }
        
        for( int i = 0; i < _vk_render_passes.size(); ++i)
        {
            vkDestroyRenderPass(_device->_logical_device, _vk_render_passes[i], nullptr);
        }
    }

    template< uint32_t NUM_ATTACHMENTS>
    void render_pass< NUM_ATTACHMENTS>::begin_render_pass(VkCommandBuffer& buffer, uint32_t swapchain_image_id)
    {
        VkRenderPassBeginInfo render_pass_create_info = {};
        render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_create_info.pNext = nullptr;
        //TODO: maybe we just need one render pass instead of using swapchain_image_id
        assert(get_vk_render_pass(swapchain_image_id) != VK_NULL_HANDLE && "call init on this render pass");
        render_pass_create_info.renderPass = get_vk_render_pass(swapchain_image_id);
        render_pass_create_info.framebuffer = get_vk_frame_buffer( swapchain_image_id);
        render_pass_create_info.renderArea.offset = { 0, 0 };
        render_pass_create_info.renderArea.extent = { (uint32_t)_dimensions.x, (uint32_t)_dimensions.y };
        
        render_pass_create_info.clearValueCount = NUM_ATTACHMENTS;
        render_pass_create_info.pClearValues = _attachment_group.get_clear_values();

        vkCmdBeginRenderPass(buffer, &render_pass_create_info, VK_SUBPASS_CONTENTS_INLINE);
        
        VkViewport viewport;
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport. width = _dimensions.x;
        viewport.height = _dimensions.y;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(buffer, 0, 1, &viewport);
        
        VkRect2D scissor;
        scissor.offset = { 0, 0};
        scissor.extent = { (uint32_t)_dimensions.x,(uint32_t)_dimensions.y};
        vkCmdSetScissor(buffer, 0, 1, &scissor);
    }



    template< uint32_t NUM_ATTACHMENTS>
    void render_pass< NUM_ATTACHMENTS>::end_render_pass(VkCommandBuffer& buffer)
    {
        vkCmdEndRenderPass(buffer);
    }

}
