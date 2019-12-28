//
//  render_pass.cpp
//  vulkan-demos
//
//  Created by Rafael Sabino on 12/28/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#include "render_pass.h"
#include <array>
#include "../shapes/obj_shape.h"
#include "depth_texture.h"
#include "../core/device.h"


using namespace vk;


render_pass::render_pass(vk::device* device, bool off_screen_rendering, glm::vec2 dimensions)
{
    _off_screen_rendering = off_screen_rendering;
    _dimensions = dimensions;
    _device = device;
}

void render_pass::set_rendering_attachments(std::array<std::vector<texture_2d*>, swapchain::NUM_SWAPCHAIN_IMAGES>& rendering_textures)
{
    _attachments = &rendering_textures;
}
void render_pass::init()
{
    assert(_attachments != nullptr && "you need to attach something to render to");
    assert(_attachments->size() <= MAX_NUMBER_OF_ATTACHMENTS);
    assert(_attachments->size() != 0);
    
    uint32_t width = _attachments->at(0)[0]->get_width();
    uint32_t height = _attachments->at(0)[0]->get_height();
    
    for( uint32_t swapchain_index = 0; swapchain_index < _attachments->size(); ++swapchain_index)
    {
        assert(_attachments[swapchain_index].size() != 0);
        
        std::array<VkAttachmentDescription, MAX_NUMBER_OF_ATTACHMENTS> attachment_descriptions {};
        std::array<VkAttachmentReference, MAX_NUMBER_OF_ATTACHMENTS> color_references {};
        
        for( uint32_t attachment_index = 0; attachment_index < _attachments[swapchain_index].size()-2; ++swapchain_index)
        {
            assert(width == _attachments->at(swapchain_index)[attachment_index]->get_width());
            assert(height == _attachments->at(swapchain_index)[attachment_index]->get_height());
            
            attachment_descriptions[attachment_index].samples = VK_SAMPLE_COUNT_1_BIT;
            attachment_descriptions[attachment_index].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachment_descriptions[attachment_index].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachment_descriptions[attachment_index].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachment_descriptions[attachment_index].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            
            attachment_descriptions[attachment_index].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachment_descriptions[attachment_index].finalLayout = _off_screen_rendering ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL :VK_IMAGE_LAYOUT_PRESENT_SRC_KHR ;
            attachment_descriptions[attachment_index].format = static_cast<VkFormat>(_attachments->at(swapchain_index)[attachment_index]->get_format());
            color_references[swapchain_index] = {attachment_index, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
        }
        
        VkAttachmentReference depth_reference {};
        VkSubpassDescription subpass = {};
        
        if(_depth_textures->size())
        {
            assert(width == _depth_textures->at(swapchain_index)->get_width());
            assert(height == _depth_textures->at(swapchain_index)->get_height());
            
            assert(_depth_textures->size() == _attachments->size() && "Each rendering pass must have it's own depth attachment");
            attachment_descriptions[_attachments[swapchain_index].size()-1] =  _depth_textures->at(swapchain_index)->get_depth_attachment();
            depth_reference.attachment = attachment_descriptions.size()-1;
            depth_reference.layout = static_cast<VkImageLayout>(_depth_textures->at(swapchain_index)->get_image_layout());
            subpass.pDepthStencilAttachment = &depth_reference;
        }

        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.pColorAttachments = color_references.data();
        subpass.colorAttachmentCount = static_cast<uint32_t>(color_references.size());
        subpass.pDepthStencilAttachment = &depth_reference;

        VkRenderPassCreateInfo render_pass_info = {};
        render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        render_pass_info.pAttachments = attachment_descriptions.data();
        render_pass_info.attachmentCount = static_cast<uint32_t>(attachment_descriptions.size());
        render_pass_info.subpassCount = 1;
        render_pass_info.pSubpasses = &subpass;
        render_pass_info.dependencyCount = 0;
        render_pass_info.pDependencies = nullptr;

        vkCreateRenderPass(_device->_logical_device, &render_pass_info, nullptr, &_vk_render_passes[swapchain_index]);

        create_frame_buffer();
    }
    

}

void render_pass::create_frame_buffer()
{
    
    for( size_t swapchain_index = 0; swapchain_index < _vk_frame_buffers.size(); ++swapchain_index)
    {
        for( size_t i = 0; i < _vk_frame_buffers.size(); ++i)
        {
            std::array<VkImageView, MAX_NUMBER_OF_ATTACHMENTS> attachment_views {};
            
            assert(_attachments->at(i).size() < MAX_NUMBER_OF_ATTACHMENTS);
            for( size_t j = 0; j < _attachments[i].size()-2; ++j)
            {
                attachment_views[j] = _attachments->at(i)[j]->_image_view;
            }
            if(_depth_textures->size() != 0)
            {
                attachment_views[_attachments[i].size()-1]  = _depth_textures->at(swapchain_index)->_image_view;
            }
            
            VkFramebufferCreateInfo framebuffer_create_info {};
            framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebuffer_create_info.pNext = nullptr;
            framebuffer_create_info.flags = 0;
            framebuffer_create_info.renderPass = _vk_render_passes[swapchain_index];
            framebuffer_create_info.attachmentCount = attachment_views.size();
            framebuffer_create_info.pAttachments = attachment_views.data();
            //there is an assumption that all attachments are the same width and height, I put some
            //asserts before which will check if this is true
            framebuffer_create_info.width = _attachments->at(0)[0]->get_width();
            framebuffer_create_info.height = _attachments->at(0)[0]->get_height();
            framebuffer_create_info.layers = 1;
            
            VkResult result = vkCreateFramebuffer(_device->_logical_device, &framebuffer_create_info, nullptr, &(_vk_frame_buffers[swapchain_index]._frame_buffer));
            ASSERT_VULKAN(result)
        }
    }

}


void render_pass::record_draw_commands(obj_shape ** shapes, size_t num_shapes)
{
}
