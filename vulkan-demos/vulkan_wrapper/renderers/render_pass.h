//
//  render_pass.hpp
//  vulkan-demos
//
//  Created by Rafael Sabino on 12/28/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#pragma once

#include <vector>
#include <array>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_SILENT_WARNINGS

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include <glm/glm.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include "../core/glfw_swapchain.h"
#include "../core/object.h"

#include "../shapes/obj_shape.h"
#include "depth_texture.h"
#include "../core/device.h"
#include "graphics_pipeline.h"

namespace vk
{
    
    //TODO: You might need another argument for number of subpasses...
    template< class TEXTURE_TYPE, uint32_t NUM_ATTACHMENTS>
    class render_pass : public object
    {
    public:
        using graphics_pipeline_type = graphics_pipeline<TEXTURE_TYPE, NUM_ATTACHMENTS>;
        using write_channels = typename graphics_pipeline_type::write_channels;
        
        static constexpr uint32_t MAX_NUMBER_OF_ATTACHMENTS = 5;
        
        /////////////////////////////////////////////////////////////////////////////////////
        class subpass_s
        {
        public:
            
            static constexpr int32_t INVALID_ATTACHMENT = -1;
            
            inline uint32_t get_id(){ return id; }
            inline bool is_active(){ return _active; }
            
            //TODO: maybe this function should be merged with set_attachment_imput
            inline void add_attachment_input(const char* parameter_name , uint32_t attachment_id)
            {
                _attachment_mapping[parameter_name] = attachment_id;
            }
            
            
            inline void set_number_of_blend_attachments( uint32_t blend_attachments)
            {
                for( int chain_id = 0; chain_id < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++chain_id)
                {
                    _pipeline[chain_id].set_number_of_blend_attachments(blend_attachments);
                }
            }
            
            inline void set_cull_mode(typename graphics_pipeline_type::cull_mode mode)
            {
                for( int chain_id = 0; chain_id < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++chain_id)
                {
                    _pipeline[chain_id].set_cull_mode(mode);
                }
            }
            void modify_attachment_blend( uint32_t index, write_channels channels, bool depth_enable)
            {
                for( int chain_id = 0; chain_id < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++chain_id)
                {
                    _pipeline[chain_id].modify_attachment_blend(index, channels, depth_enable);
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
                int i = 0;
                
                //TODO: this code will likely change when we start using subpass dependencies
                for( std::pair<const char*, uint32_t> iter : _attachment_mapping)
                {
                    _color_references[i].attachment = iter.second;
                    _color_references[i].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                    ++i;
                }
                
                _subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
                _subpass_description.pColorAttachments = _color_references.data();
                _subpass_description.colorAttachmentCount = i;
            }
            
            inline void set_image_sampler(texture_3d& texture, const char* parameter_name,
                                          visual_material::parameter_stage parameter_stage, uint32_t binding,  resource::usage_type usage)
            {
                for( int chain_id = 0; chain_id < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++chain_id)
                {
                    _pipeline[chain_id].set_image_sampler(&texture, parameter_name, parameter_stage, binding, usage);
                    //_material[0]->set_image_sampler(&texture, parameter_name, parameter_stage, binding, usage);
                }
            }
            
            inline void set_image_sampler(texture_2d& texture, const char* parameter_name,
                                          visual_material::parameter_stage parameter_stage,  uint32_t binding, resource::usage_type usage)
            {
                for( int chain_id = 0; chain_id < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++chain_id)
                {
                    _pipeline[chain_id].set_image_sampler(texture, parameter_name, parameter_stage, binding, usage);
                    //_material[0]->set_image_sampler(&texture, parameter_name, parameter_stage, binding, usage);
                }
            }
            inline void set_image_sampler(std::array<depth_texture, glfw_swapchain::NUM_SWAPCHAIN_IMAGES>& textures, const char* parameter_name,
                                          visual_material::parameter_stage parameter_stage, uint32_t binding,  resource::usage_type usage)
            {
                for( int chain_id = 0; chain_id < textures.size(); ++chain_id)
                {
                    _pipeline[chain_id].set_image_sampler(textures[chain_id], parameter_name, parameter_stage, binding, usage) ;
                }
            }
            
            inline void set_image_sampler(std::array<render_texture, glfw_swapchain::NUM_SWAPCHAIN_IMAGES>& textures, const char* parameter_name,
                                          visual_material::parameter_stage parameter_stage, uint32_t binding, resource::usage_type usage)
            {
                for( int chain_id = 0; chain_id < textures.size(); ++chain_id)
                {
                    _pipeline[chain_id].set_image_sampler(textures[chain_id], parameter_name, parameter_stage, binding, usage) ;
                }
            }
            
            inline void init_parameter(const char* parameter_name, visual_material::parameter_stage stage,  float value, int binding)
            {
                for( int chain_id = 0; chain_id < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++chain_id)
                {
                    _pipeline[chain_id].init_parameter(parameter_name, stage, value, binding);
                }
            };
            
            inline void init_parameter(const char* parameter_name, visual_material::parameter_stage stage,  int value, int binding)
            {
                for( int chain_id = 0; chain_id < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++chain_id)
                {
                    _pipeline[chain_id].init_parameter(parameter_name, stage, value, binding);
                }
            };
            inline void init_parameter(const char* parameter_name, visual_material::parameter_stage stage,  glm::vec3 value, int binding)
            {
                for( int chain_id = 0; chain_id < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++chain_id)
                {
                    _pipeline[chain_id].init_parameter(parameter_name, stage, value, binding);
                }
            };
            inline void init_parameter(const char* parameter_name, visual_material::parameter_stage stage,  glm::vec4 value, int binding)
            {
                for( int chain_id = 0; chain_id < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++chain_id)
                {
                    _pipeline[chain_id].init_parameter(parameter_name, stage, value, binding);
                }
            };
            inline void init_parameter(const char* parameter_name, visual_material::parameter_stage stage,  glm::vec2 value, int binding)
            {
                for( int chain_id = 0; chain_id < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++chain_id)
                {
                    _pipeline[chain_id].init_parameter(parameter_name, stage, value, binding);
                }
            }
            inline void init_parameter(const char* parameter_name, visual_material::parameter_stage stage, glm::mat4 value, int binding)
            {
                for( int chain_id = 0; chain_id < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++chain_id)
                {
                    _pipeline[chain_id].init_parameter(parameter_name, stage, value, binding);
                }
            }
            
            inline void init_parameter(const char* parameter_name, visual_material::parameter_stage stage,
                                       glm::vec4* vecs,  size_t num_vectors, int binding)
            {
                for( int chain_id = 0; chain_id < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++chain_id)
                {
                    _pipeline[chain_id].init_parameter(parameter_name, stage, vecs, num_vectors, binding);
                }
            }
            
            inline void init_dynamic_params(const char* parameter_name, visual_material::parameter_stage stage,
                                            glm::mat4& val, size_t num_objs, int binding)
            {
                
                for( int chain_id = 0; chain_id < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++chain_id)
                {
                    for( int j = 0; j < num_objs; ++j)
                        _pipeline[chain_id].get_dynamic_parameters(stage, binding)[j][parameter_name] = val;
                }
            }
            
            inline void set_image_sampler(std::array<texture_3d, glfw_swapchain::NUM_SWAPCHAIN_IMAGES>& textures, const char* parameter_name,
                                          visual_material::parameter_stage parameter_stage, uint32_t binding, resource::usage_type usage)
            {
                for( int chain_id = 0; chain_id < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++chain_id)
                {
                    _pipeline[chain_id].set_image_sampler(textures[chain_id], parameter_name, parameter_stage, binding, usage) ;
                }
            }
            
            void set_material( visual_mat_shared_ptr& material)
            {
                for( int chain_id = 0; chain_id < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++chain_id)
                {
                    //TODO: how will you know if this pipeline needs to be initialized
                    _pipeline[chain_id].set_material(material);
                }
            }
            
            inline VkSubpassDescription get_subpass_description() { return _subpass_description; }
            
            void commit_parameters_to_gpu(uint32_t swapchain_id)
            {
                _pipeline[swapchain_id].commit_parameters_to_gpu();
            }
            
            inline graphics_pipeline_type& get_pipeline(uint32_t swapchain_id ){ return _pipeline[swapchain_id]; }
            
            void create(std::array<VkRenderPass, glfw_swapchain::NUM_SWAPCHAIN_IMAGES>& vk_render_passes)
            {
                for(uint32_t i = 0; i < _pipeline.size(); ++i)
                {
                    _pipeline[i].create(vk_render_passes[i]);
                }
            }
            
            inline void set_active(){ _active = true; }
            const char* _name = nullptr;
        private:

            VkSubpassDescription _subpass_description {};
            bool _active = false;
            uint32_t id = INVALID_ATTACHMENT;
            ordered_map<const char*, uint32_t> _attachment_mapping;
            std::array<VkAttachmentReference, MAX_NUMBER_OF_ATTACHMENTS> _color_references {};
            std::array<graphics_pipeline_type, glfw_swapchain::NUM_SWAPCHAIN_IMAGES> _pipeline;
            device* _device = nullptr;
        };
        /////////////////////////////////////////////////////////////////////////////////////
        
        void commit_parameters_to_gpu(uint32_t swapchain_id)
        {
            for( int subpass_id = 0; subpass_id < _subpasses.size(); ++subpass_id)
            {
                if(!_subpasses[subpass_id].is_active()) break;
                //TODO: make _vk_render_passes of size 1
                _subpasses[subpass_id].commit_parameters_to_gpu(swapchain_id);
            }
        }
        
        void begin_command_recording(VkCommandBuffer& buffer, uint32_t swapchain_image_id, uint32_t subpass_id);
        void end_command_recording();
        

        render_pass(){}
        render_pass(device* device, glm::vec2 dimensions);
        
        void set_clear_depth( glm::vec2 depth_stencil)
        {
            assert(_depth_enable && "you must enable depth for this to take effect");
            //note: depth value is always the last one...
            _clear_values[_clear_values.size()-1].depthStencil = {depth_stencil.x, (uint32_t)depth_stencil.y};
            
        }
        
        void set_clear_attachments_colors( glm::vec4 color)
        {
            //note: -2 because the last one is depth attachment
            for(int i = 0; i < _clear_values.size()-2; ++i)
            {
                _clear_values[i].color = {color.x, color.y, color.z, color.w};
            }
        }
        
        void set_rendering_attachments(std::array<std::array<TEXTURE_TYPE,glfw_swapchain::NUM_SWAPCHAIN_IMAGES>, NUM_ATTACHMENTS>& rendering_texture);
        
        //TODO: re-name this to create_depth_attachment()
        void set_depth_enable(bool enable){ _depth_enable = enable; };
        void init();
        
        void record_draw_commands(obj_shape**, size_t num_shapes);
        
        inline VkRenderPass& get_vk_render_pass(uint32_t i)
        {
            assert( i < _vk_render_passes.size());
            return _vk_render_passes[i];

        };
        
        inline VkImageView& get_vk_depth_image_view(uint32_t i)
        {
            assert( i < _depth_textures.size());
            return _depth_textures[i]._image_view;
            
        }
        
        inline VkFramebuffer& get_vk_frame_buffer( uint32_t swapchain_id)
        {
            assert(_vk_frame_buffer_infos.size() > swapchain_id);
            
            return _vk_frame_buffer_infos[swapchain_id];
        }
        
        inline std::array<depth_texture, glfw_swapchain::NUM_SWAPCHAIN_IMAGES>& get_depth_textures(){ return _depth_textures; }
        
        inline void set_device(device* device)
        {
            _device = device;
        }
        
        inline void set_offscreen_rendering(bool offscreen ){ _off_screen_rendering = offscreen; }
        
        inline graphics_pipeline_type& get_pipeline( uint32_t swapchain_id, uint32_t subpass_id){ return get_subpass(subpass_id).get_pipeline(swapchain_id);}
        inline subpass_s& add_subpass(visual_mat_shared_ptr mat,  const char* name = "" )
        {
            _subpasses[_num_subpasses]._name = name;
            _subpasses[_num_subpasses].set_active();
            _subpasses[_num_subpasses].set_device(_device);
            _subpasses[_num_subpasses].set_material(mat);
            
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
        
        inline subpass_s& get_subpass(uint32_t subpass_id){ return _subpasses[subpass_id]; }
        
        inline void create()
        {
            init();
            for( int subpass_id = 0; subpass_id < _num_subpasses; ++subpass_id)
            {
                if(!_subpasses[subpass_id].is_active()) break;
                //TODO: make _vk_render_passes of size 1
                _subpasses[subpass_id].create(_vk_render_passes);
            }
        }
        void destroy() override;
        
    private:
        
        void create_frame_buffers();
        
    private:
        
        static constexpr unsigned int MAX_SUBPASSES = 20u;
        
        std::array<std::array<TEXTURE_TYPE, vk::glfw_swapchain::NUM_SWAPCHAIN_IMAGES>, NUM_ATTACHMENTS>*  _attachments = nullptr;
        std::array<depth_texture, vk::glfw_swapchain::NUM_SWAPCHAIN_IMAGES>             _depth_textures;
        std::array<VkRenderPass,glfw_swapchain::NUM_SWAPCHAIN_IMAGES>   _vk_render_passes {};
        std::array<VkFramebuffer, glfw_swapchain::NUM_SWAPCHAIN_IMAGES> _vk_frame_buffer_infos {};
        
        std::array<subpass_s, MAX_SUBPASSES> _subpasses {};
        
        bool _off_screen_rendering = false;
        bool _depth_enable = true;
        
        static_assert(MAX_NUMBER_OF_ATTACHMENTS > NUM_ATTACHMENTS, "Number of attachments in your render pass excees what we can handle, increase limit??");

        std::array<VkClearValue,NUM_ATTACHMENTS + 1> _clear_values {};
        glm::vec2 _dimensions {};
        device* _device = nullptr;
        VkCommandBuffer* _recording_buffer = VK_NULL_HANDLE;
        uint32_t _num_subpasses = 0;
        
    };

    template<class TEXTURE_TYPE, uint32_t NUM_ATTACHMENTS>
    render_pass<TEXTURE_TYPE, NUM_ATTACHMENTS>::render_pass(vk::device* device,  glm::vec2 dimensions)
    {
        _dimensions = dimensions;
        _device = device;
        
        for( int subpass_id = 0; subpass_id < _subpasses.size(); ++subpass_id )
        {
            _subpasses[subpass_id].set_device(device);
            _subpasses[subpass_id].set_viewport(dimensions);
        }
    }

    template <class TEXTURE_TYPE, uint32_t NUM_ATTACHMENTS>
    void render_pass<TEXTURE_TYPE, NUM_ATTACHMENTS>::set_rendering_attachments(std::array<std::array<TEXTURE_TYPE, glfw_swapchain::NUM_SWAPCHAIN_IMAGES>,
                                                                               NUM_ATTACHMENTS>& rendering_textures)
    {
        _attachments = &rendering_textures;
    }
    
    template <class TEXTURE_TYPE, uint32_t NUM_ATTACHMENTS>
    void render_pass<TEXTURE_TYPE, NUM_ATTACHMENTS>::init()
    {
        assert(_attachments != nullptr && "you need to attach something to render to");
        assert(_attachments->size() <= MAX_NUMBER_OF_ATTACHMENTS);
        assert(_attachments->size() != 0);
        
        //notes: attachments all have the same width and height
        uint32_t width = _attachments->at(0)[0].get_width();
        uint32_t height = _attachments->at(0)[0].get_height();
        
        image::filter f = _attachments->at(0)[0].get_filter();
        if(_depth_enable )
        {
            for( int i = 0; i < _depth_textures.size(); ++i)
            {
                _depth_textures[i].set_device(_device);
                _depth_textures[i].set_dimensions(width,height, 1.0f);
                _depth_textures[i].set_filter(f);
                _depth_textures[i].set_write_to_texture(_off_screen_rendering);
                _depth_textures[i].init();
            }
        }

        
        for( int swapchain_index = 0; swapchain_index < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++swapchain_index)
        {
            
            std::array<VkAttachmentDescription, MAX_NUMBER_OF_ATTACHMENTS> attachment_descriptions {};
            //an excellent explanation of what the heck are these attachment references:
            //https://stackoverflow.com/questions/49652207/what-is-the-purpose-of-vkattachmentreference
            
            //here is article about subpasses and input attachments and how they are all tied togethere
            //https://www.saschawillems.de/blog/2018/07/19/vulkan-input-attachments-and-sub-passes/
            uint32_t attachment_id = 0;
            assert(_attachments->at(attachment_id).size() == glfw_swapchain::NUM_SWAPCHAIN_IMAGES);
            
            for( ; attachment_id < _attachments->size(); ++attachment_id)
            {
                assert(_attachments[attachment_id].size() != 0);
                assert(width == _attachments->at(attachment_id)[swapchain_index].get_width());
                assert(height == _attachments->at(attachment_id)[swapchain_index].get_height());
                
                attachment_descriptions[attachment_id].samples = VK_SAMPLE_COUNT_1_BIT;
                attachment_descriptions[attachment_id].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                attachment_descriptions[attachment_id].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                attachment_descriptions[attachment_id].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                attachment_descriptions[attachment_id].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                
                attachment_descriptions[attachment_id].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                attachment_descriptions[attachment_id].finalLayout = _off_screen_rendering ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL :VK_IMAGE_LAYOUT_PRESENT_SRC_KHR ;
                attachment_descriptions[attachment_id].format = static_cast<VkFormat>(_attachments->at(attachment_id)[swapchain_index].get_format());
                
            }
            
            assert(attachment_id != 0);
            
            std::array<VkSubpassDescription, MAX_SUBPASSES> subpass {};
            
            assert(_subpasses[0].is_active() && "You need at least one subpass for rendering to occur");
            int subpass_id = 0;
            while(_subpasses[subpass_id].is_active())
            {
                _subpasses[subpass_id].init();
                subpass[subpass_id] = _subpasses[subpass_id].get_subpass_description();
                
                //an excellent explanation of what the heck are these attachment references:
                //https://stackoverflow.com/questions/49652207/what-is-the-purpose-of-vkattachmentreference
                
                VkAttachmentReference depth_reference {};

                 if(_depth_enable)
                {
                    assert(width == _depth_textures[swapchain_index].get_width());
                    assert(height == _depth_textures[swapchain_index].get_height());
                    
                    //note: in this code, the last attachement is the depth
                    attachment_descriptions[attachment_id] =  _depth_textures[swapchain_index].get_depth_attachment();
                    depth_reference.attachment = attachment_id;
                    depth_reference.layout = static_cast<VkImageLayout>(_depth_textures[swapchain_index].get_image_layout());
                    subpass[subpass_id].pDepthStencilAttachment = &depth_reference;
                }

                //note: for a great explanation of VK_SUBPASS_EXTERNAL:
                //https://stackoverflow.com/questions/53984863/what-exactly-is-vk-subpass-external?rq=1
                
                //note: because we state the images initial layout upon entering the renderpass and the layouts the subpasses
                //need for them to do their work, transitions are implicit and will happen automatically as the renderpass excecutes.
                
                ++subpass_id;
            }
            attachment_id = _depth_enable ? attachment_id + 1 : attachment_id;

            VkRenderPassCreateInfo render_pass_info = {};
            render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            render_pass_info.pAttachments = attachment_descriptions.data();
            render_pass_info.attachmentCount =attachment_id;
            render_pass_info.subpassCount = subpass_id;
            render_pass_info.pSubpasses = subpass.data();
            render_pass_info.dependencyCount = 0;
            render_pass_info.pDependencies = nullptr;

            VkResult result = vkCreateRenderPass(_device->_logical_device, &render_pass_info, nullptr, &_vk_render_passes[swapchain_index]);
            ASSERT_VULKAN(result);
        }
        
        create_frame_buffers();
    }

    template <class TEXTURE_TYPE, uint32_t NUM_ATTACHMENTS>
    void render_pass<TEXTURE_TYPE, NUM_ATTACHMENTS>::create_frame_buffers()
    {
        
        for( uint32_t swapchain_id = 0; swapchain_id < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++swapchain_id)
        {
            std::array<VkImageView, MAX_NUMBER_OF_ATTACHMENTS> attachment_views {};
            
            assert(_attachments->size() < MAX_NUMBER_OF_ATTACHMENTS);
            uint32_t num_views = 0;
            //add all num views for this swapchain id
            for( ; num_views < _attachments->size(); ++num_views)
            {
                assert(_attachments->at(num_views)[swapchain_id]._image_view != VK_NULL_HANDLE && "did you initialize this image?");
                attachment_views[num_views] = _attachments->at(num_views)[swapchain_id]._image_view;
            }
            if(_depth_enable)
            {
                assert(_depth_textures[swapchain_id]._image_view != VK_NULL_HANDLE);
                //the render pass assume this as well...
                attachment_views[num_views]  = _depth_textures[swapchain_id]._image_view;
                num_views++;
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
            framebuffer_create_info.width = _attachments->at(0)[0].get_width();
            framebuffer_create_info.height = _attachments->at(0)[0].get_height();
            framebuffer_create_info.layers = 1;
            
            VkResult result = vkCreateFramebuffer(_device->_logical_device, &framebuffer_create_info, nullptr, &(_vk_frame_buffer_infos[swapchain_id]));
            ASSERT_VULKAN(result)
        }
    }

    template <class TEXTURE_TYPE, uint32_t NUM_ATTACHMENTS>
    void render_pass<TEXTURE_TYPE, NUM_ATTACHMENTS>::destroy()
    {
        for( int i =0 ; i < _vk_frame_buffer_infos.size(); ++i)
        {
            vkDestroyFramebuffer(_device->_logical_device, _vk_frame_buffer_infos[i], nullptr);
        }
        
        for( int i = 0; i < _vk_render_passes.size(); ++i)
        {
            vkDestroyRenderPass(_device->_logical_device, _vk_render_passes[i], nullptr);
        }
    }
    template<class RENDER_TEXTURE_TYPE, uint32_t NUM_ATTACHMENTS>
    void render_pass<RENDER_TEXTURE_TYPE, NUM_ATTACHMENTS>::begin_command_recording(VkCommandBuffer& buffer, uint32_t swapchain_image_id, uint32_t subpass_id)
    {
        assert(_recording_buffer == VK_NULL_HANDLE && "you shouldn't call begin_command_recording before calling end_command_recording");
        _recording_buffer = &buffer;
        
        VkCommandBufferBeginInfo command_buffer_begin_info {};
        command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_begin_info.pNext = nullptr;
        command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        command_buffer_begin_info.pInheritanceInfo = nullptr;
        
        VkResult result = vkBeginCommandBuffer(buffer, &command_buffer_begin_info);
        ASSERT_VULKAN(result);
        
        VkRenderPassBeginInfo render_pass_create_info = {};
        render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_create_info.pNext = nullptr;
        //TODO: maybe we just need one render pass instead of using swapchain_image_id
        render_pass_create_info.renderPass = get_vk_render_pass(swapchain_image_id);
        render_pass_create_info.framebuffer = get_vk_frame_buffer( swapchain_image_id);
        render_pass_create_info.renderArea.offset = { 0, 0 };
        render_pass_create_info.renderArea.extent = { (uint32_t)_dimensions.x, (uint32_t)_dimensions.y };
        
        render_pass_create_info.clearValueCount = _depth_enable ? static_cast<uint32_t>(_clear_values.size()) : static_cast<uint32_t>(_clear_values.size() -1);
        render_pass_create_info.pClearValues = _clear_values.data();
        
        vkCmdBeginRenderPass(buffer, &render_pass_create_info, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,_subpasses[subpass_id].get_pipeline(swapchain_image_id).get_vk_pipeline());

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

    template<class RENDER_TEXTURE_TYPE, uint32_t NUM_ATTACHMENTS>
    void render_pass<RENDER_TEXTURE_TYPE, NUM_ATTACHMENTS>::end_command_recording()
    {
        assert(_recording_buffer != nullptr && "you must call begin_command_recording");
        
        vkCmdEndRenderPass(*_recording_buffer);
        vkEndCommandBuffer(*_recording_buffer);
        
        _recording_buffer = nullptr;
    }
}
