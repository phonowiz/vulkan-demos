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

namespace vk
{
    
    template< class TEXTURE_TYPE, uint32_t NUM_ATTACHMENTS>
    class render_pass : public object
    {
    public:
        
        render_pass(){}
        render_pass(device* device, glm::vec2 dimensions);
        
        void set_rendering_attachments(std::array<std::array<TEXTURE_TYPE,glfw_swapchain::NUM_SWAPCHAIN_IMAGES>, NUM_ATTACHMENTS>& rendering_texture);
        
        void set_depth_enable(bool enable){ _depth_enable = enable; };
        void init();
        
        void record_draw_commands(obj_shape**, size_t num_shapes);
        
        inline std::array<VkRenderPass, glfw_swapchain::NUM_SWAPCHAIN_IMAGES>& get_render_passes(){ return _vk_render_passes; }
        
        inline VkRenderPass get_vk_render_pass(size_t i)
        {
            assert( i < _vk_render_passes.size());
            return _vk_render_passes[i];
            
        };
        
        inline VkImageView get_vk_depth_image_view(size_t i)
        {
            assert( i < _depth_textures.size());
            return _depth_textures[i]._image_view;
            
        }
        
        inline VkFramebuffer get_vk_frame_buffer(size_t i)
        {
            assert(_vk_frame_buffer_infos.size() > i);
            
            return _vk_frame_buffer_infos[i];
        }
        
        inline std::array<depth_texture, glfw_swapchain::NUM_SWAPCHAIN_IMAGES>& get_depth_textures(){ return _depth_textures; }
        
        inline void set_device(device* device){ _device = device; }
        
        inline void set_offscreen_rendering(bool offscreen ){ _off_screen_rendering = offscreen; }
        
        void destroy() override;
    private:
        
        void create_frame_buffers();
        
    private:
        
        //note: we rely on NUM_SWAPCHAIN_IMAGES to declare in these arrays, in the future if that is no longer
        //the way to go, templetize this.
        
        std::array<std::array<TEXTURE_TYPE, vk::glfw_swapchain::NUM_SWAPCHAIN_IMAGES>, NUM_ATTACHMENTS>*  _attachments = nullptr;
        std::array<depth_texture, vk::glfw_swapchain::NUM_SWAPCHAIN_IMAGES>             _depth_textures;
        std::array<VkRenderPass, glfw_swapchain::NUM_SWAPCHAIN_IMAGES>       _vk_render_passes {};
        std::array<VkFramebuffer, glfw_swapchain::NUM_SWAPCHAIN_IMAGES>  _vk_frame_buffer_infos {};
        
        bool _off_screen_rendering = false;
        bool _depth_enable = true;

        static constexpr uint32_t MAX_NUMBER_OF_ATTACHMENTS = 5;
        
        static_assert(MAX_NUMBER_OF_ATTACHMENTS > NUM_ATTACHMENTS, "Number of attachments in your render pass excees what we can handle, increase limit??");

        glm::vec2 _dimensions {};
        device* _device = nullptr;
        
    };

    template<class TEXTURE_TYPE, uint32_t NUM_ATTACHMENTS>
    render_pass<TEXTURE_TYPE, NUM_ATTACHMENTS>::render_pass(vk::device* device,  glm::vec2 dimensions)
    {
        _dimensions = dimensions;
        _device = device;
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
            std::array<VkAttachmentReference, MAX_NUMBER_OF_ATTACHMENTS> color_references {};
            
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
                
                //this is the layout the attahment will be used during the subpass.  The driver decides if there should be a
                //transition or not given the 'initialLayout' specified in the attachment description
                
                //the first integer in the instruction is the attachment location specified in the shader
                color_references[attachment_id] = {attachment_id, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
            }
            
            assert(attachment_id != 0);
            VkSubpassDescription subpass {};
            subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass.pColorAttachments = color_references.data();
            subpass.colorAttachmentCount = attachment_id;
            
            //an excellent explanation of what the heck are these attachment references:
            //https://stackoverflow.com/questions/49652207/what-is-the-purpose-of-vkattachmentreference
            
            VkAttachmentReference depth_reference {};

             if(_depth_enable)
            {
                assert(width == _depth_textures[swapchain_index].get_width());
                assert(height == _depth_textures[swapchain_index].get_height());
                
                //note: in this code, the last attachement is the depth
                attachment_descriptions[attachment_id] =  _depth_textures[swapchain_index].get_depth_attachment();
                depth_reference.attachment = attachment_id;//attachment_descriptions.size();
                depth_reference.layout = static_cast<VkImageLayout>(_depth_textures[swapchain_index].get_image_layout());
                subpass.pDepthStencilAttachment = &depth_reference;
                ++attachment_id;
            }

            //note: for a great explanation of VK_SUBPASS_EXTERNAL:
            //https://stackoverflow.com/questions/53984863/what-exactly-is-vk-subpass-external?rq=1
            
            //note: because we state the images initial layout upon entering the renderpass and the layouts the subpasses
            //need for them to do their work, transitions are implicit and will happen automatically as the renderpass excecutes.
            
            
            VkRenderPassCreateInfo render_pass_info = {};
            render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            render_pass_info.pAttachments = attachment_descriptions.data();
            render_pass_info.attachmentCount =attachment_id;
            render_pass_info.subpassCount = 1;
            render_pass_info.pSubpasses = &subpass;
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
        
        for( size_t swapchain_index = 0; swapchain_index < _vk_frame_buffer_infos.size(); ++swapchain_index)
        {
            std::array<VkImageView, MAX_NUMBER_OF_ATTACHMENTS> attachment_views {};
            
            assert(_attachments->size() < MAX_NUMBER_OF_ATTACHMENTS);
            uint32_t num_views = 0;
            for( ; num_views < _attachments->size(); ++num_views)
            {
                assert(_attachments->at(num_views)[swapchain_index]._image_view != VK_NULL_HANDLE && "did you initialize this image?");
                attachment_views[num_views] = _attachments->at(num_views)[swapchain_index]._image_view;
            }
            if(_depth_enable)
            {
                assert(_depth_textures[swapchain_index]._image_view != VK_NULL_HANDLE);
                //the render pass assume this as well...
                attachment_views[num_views]  = _depth_textures[swapchain_index]._image_view;
                num_views++;
            }
            
            VkFramebufferCreateInfo framebuffer_create_info {};
            framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebuffer_create_info.pNext = nullptr;
            framebuffer_create_info.flags = 0;
            framebuffer_create_info.renderPass = _vk_render_passes[swapchain_index];
            framebuffer_create_info.attachmentCount = num_views;
            framebuffer_create_info.pAttachments = attachment_views.data();
            //there is an assumption that all attachments are the same width and height, I put some
            //asserts before which will check if this is true
            framebuffer_create_info.width = _attachments->at(0)[0].get_width();
            framebuffer_create_info.height = _attachments->at(0)[0].get_height();
            framebuffer_create_info.layers = 1;
            
            VkResult result = vkCreateFramebuffer(_device->_logical_device, &framebuffer_create_info, nullptr, &(_vk_frame_buffer_infos[swapchain_index]));
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
}
