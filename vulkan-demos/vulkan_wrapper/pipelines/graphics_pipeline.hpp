//
//  graphics_pipeline.cpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 3/22/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#include "graphics_pipeline.h"
#include "vertex.h"

template<class RENDER_TEXTURE_TYPE, uint32_t NUM_ATTACHMENTS>
void graphics_pipeline<RENDER_TEXTURE_TYPE, NUM_ATTACHMENTS>::init_blend_attachments()
{
    for( uint32_t i = 0; i < BLEND_ATTACHMENTS; ++i)
    {
        _blend_attachments[i].blendEnable = VK_TRUE;
        _blend_attachments[i].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        _blend_attachments[i].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        _blend_attachments[i].colorBlendOp = VK_BLEND_OP_ADD;
        _blend_attachments[i].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        _blend_attachments[i].dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        _blend_attachments[i].alphaBlendOp = VK_BLEND_OP_ADD;
        _blend_attachments[i].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    }
}

template<class RENDER_TEXTURE_TYPE, uint32_t NUM_ATTACHMENTS>
void graphics_pipeline<RENDER_TEXTURE_TYPE, NUM_ATTACHMENTS>::begin_command_recording(int swapchain_image_id, glfw_swapchain& swapchain, VkCommandBuffer* buffer)
{
    VkRenderPassBeginInfo render_pass_create_info = {};
    render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_create_info.pNext = nullptr;
    render_pass_create_info.renderPass = _render_pass->get_vk_render_pass(swapchain_image_id);
    render_pass_create_info.framebuffer = _render_pass->get_vk_frame_buffer(swapchain_image_id);
    render_pass_create_info.renderArea.offset = { 0, 0 };
    render_pass_create_info.renderArea.extent = { swapchain.get_vk_swap_extent().width,
        swapchain.get_vk_swap_extent().height };
    VkClearValue clear_value = {0.0f, 0.0f, 0.0f, 1.0f};
    VkClearValue depth_clear_value = {1.0f, 0.0f};
    
    std::array<VkClearValue,2> clear_values;
    clear_values[0] = clear_value;
    clear_values[1] = depth_clear_value;
    
    render_pass_create_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
    render_pass_create_info.pClearValues = clear_values.data();
    
    vkCmdBeginRenderPass(buffer[swapchain_image_id], &render_pass_create_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(buffer[swapchain_image_id], VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline[swapchain_image_id]);
    
    VkViewport viewport;
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport. width = swapchain.get_vk_swap_extent().width;
    viewport.height = swapchain.get_vk_swap_extent().height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(buffer[swapchain_image_id], 0, 1, &viewport);
    
    VkRect2D scissor;
    scissor.offset = { 0, 0};
    scissor.extent = { swapchain.get_vk_swap_extent().width,
        swapchain.get_vk_swap_extent().height};
    vkCmdSetScissor(buffer[swapchain_image_id], 0, 1, &scissor);
}

template< class TEXTURE_TYPE, uint32_t NUM_ATTACHMENTS>
void graphics_pipeline<TEXTURE_TYPE, NUM_ATTACHMENTS>::create( )
{

    _render_pass.init();
    
    auto vertex_binding_description = vertex::get_binding_description();
    auto vertex_attribute_descriptos = vertex::get_attribute_descriptions();

    for( int chain_index = 0; chain_index < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++chain_index)
    {
        //note: this call guarantees that material resources are ready to create a pipeline
        _material[chain_index]->commit_parameters_to_gpu();
        
        VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info {};
        vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertex_input_state_create_info.pNext = nullptr;
        vertex_input_state_create_info.flags = 0;
        vertex_input_state_create_info.vertexBindingDescriptionCount = 1;
        vertex_input_state_create_info.pVertexBindingDescriptions = &vertex_binding_description;
        vertex_input_state_create_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_attribute_descriptos.size());
        vertex_input_state_create_info.pVertexAttributeDescriptions = vertex_attribute_descriptos.data();


        VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info {};
        input_assembly_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        input_assembly_create_info.pNext = nullptr;
        input_assembly_create_info.flags = 0;
        input_assembly_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        input_assembly_create_info.primitiveRestartEnable = VK_FALSE;

        VkViewport viewport;
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = _width;
        viewport.height = _height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor;
        scissor.offset = { 0, 0 };
        scissor.extent = { _width, _height };

        VkPipelineViewportStateCreateInfo viewport_state_create_info {};
        viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewport_state_create_info.pNext = nullptr;
        viewport_state_create_info.flags = 0;
        viewport_state_create_info.viewportCount = 1;
        viewport_state_create_info.pViewports = &viewport;
        viewport_state_create_info.scissorCount = 1;
        viewport_state_create_info.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo rasterization_state_create_info {};
        rasterization_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterization_state_create_info.pNext = nullptr;
        rasterization_state_create_info.flags = 0;
        rasterization_state_create_info.depthClampEnable = VK_FALSE;
        rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;
        rasterization_state_create_info.polygonMode = VK_POLYGON_MODE_FILL;
        rasterization_state_create_info.cullMode = static_cast<VkCullModeFlagBits>(_cull_mode);

        rasterization_state_create_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterization_state_create_info.depthBiasEnable = VK_FALSE;
        rasterization_state_create_info.depthBiasConstantFactor = 0.0f;
        rasterization_state_create_info.depthBiasClamp = 0.0f;
        rasterization_state_create_info.depthBiasSlopeFactor = 0.0f;
        rasterization_state_create_info.lineWidth = 1.0f;

        VkPipelineMultisampleStateCreateInfo multisample_state_create_info {};
        multisample_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisample_state_create_info.pNext = nullptr;
        multisample_state_create_info.flags = 0;
        multisample_state_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisample_state_create_info.sampleShadingEnable = VK_FALSE;
        multisample_state_create_info.minSampleShading = 1.0f;
        multisample_state_create_info.pSampleMask = nullptr;
        multisample_state_create_info.alphaToCoverageEnable = VK_FALSE;
        multisample_state_create_info.alphaToOneEnable = VK_FALSE;

        VkPipelineDepthStencilStateCreateInfo depth_stencil_create_info = {};

        depth_stencil_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depth_stencil_create_info.pNext = nullptr;
        depth_stencil_create_info.flags = 0;
        depth_stencil_create_info.depthTestEnable =  static_cast<VkBool32>(_depth_enable);
        depth_stencil_create_info.depthWriteEnable = static_cast<VkBool32>(_depth_enable);
        depth_stencil_create_info.depthCompareOp = VK_COMPARE_OP_LESS;

        //todo:depth bounds uses the min/max depth bounds below to see if the fragment is within the bounding box
        //we are currently not using this feature
        depth_stencil_create_info.depthBoundsTestEnable = VK_TRUE;
        depth_stencil_create_info.stencilTestEnable = VK_TRUE;
        depth_stencil_create_info.front = {};
        depth_stencil_create_info.back = {};
        depth_stencil_create_info.minDepthBounds = 0.0f;
        depth_stencil_create_info.maxDepthBounds = 1.0f;

        VkPipelineColorBlendStateCreateInfo color_blend_create_info {};
        color_blend_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        color_blend_create_info.pNext = nullptr;
        color_blend_create_info.flags = 0;
        color_blend_create_info.logicOpEnable = VK_FALSE;
        color_blend_create_info.logicOp = VK_LOGIC_OP_NO_OP;
        color_blend_create_info.attachmentCount = _num_blend_attachments;
        color_blend_create_info.pAttachments = _blend_attachments.data();
        color_blend_create_info.blendConstants[0] = 0.0f;
        color_blend_create_info.blendConstants[1] = 0.0f;
        color_blend_create_info.blendConstants[2] = 0.0f;
        color_blend_create_info.blendConstants[3] = 0.0f;


        VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
        pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipeline_layout_create_info.pNext = nullptr;
        pipeline_layout_create_info.flags = 0;
        pipeline_layout_create_info.setLayoutCount = _material[chain_index]->descriptor_set_present() ? 1 : 0;
        pipeline_layout_create_info.pSetLayouts = _material[chain_index]->get_descriptor_set_layout();
        pipeline_layout_create_info.pushConstantRangeCount = 0;
        pipeline_layout_create_info.pPushConstantRanges = nullptr;

        VkResult result = vkCreatePipelineLayout(_device->_logical_device, &pipeline_layout_create_info, nullptr, &_pipeline_layout[chain_index]);
        ASSERT_VULKAN(result);

        VkDynamicState dynamic_state[] = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        VkPipelineDynamicStateCreateInfo dynamic_state_create_info {};
        dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamic_state_create_info.pNext = nullptr;
        dynamic_state_create_info.flags = 0;
        dynamic_state_create_info.dynamicStateCount = 2;
        dynamic_state_create_info.pDynamicStates = dynamic_state;

        VkGraphicsPipelineCreateInfo pipeline_create_info {};
        pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline_create_info.pNext = nullptr;
        pipeline_create_info.flags = 0;
        pipeline_create_info.stageCount = static_cast<uint32_t>(_material[chain_index]->get_shader_stages_size());
        pipeline_create_info.pStages = _material[chain_index]->get_shader_stages();
        pipeline_create_info.pVertexInputState = &vertex_input_state_create_info;
        pipeline_create_info.pInputAssemblyState = &input_assembly_create_info;
        pipeline_create_info.pTessellationState = nullptr;
        pipeline_create_info.pViewportState = &viewport_state_create_info;
        pipeline_create_info.pRasterizationState = &rasterization_state_create_info;
        pipeline_create_info.pMultisampleState = &multisample_state_create_info;
        pipeline_create_info.pDepthStencilState = &depth_stencil_create_info;
        pipeline_create_info.pColorBlendState = &color_blend_create_info;
        pipeline_create_info.pDynamicState = &dynamic_state_create_info;
        pipeline_create_info.layout = _pipeline_layout[chain_index];
        pipeline_create_info.renderPass = _render_pass.get_vk_render_pass(chain_index);
        pipeline_create_info.subpass = 0;
        pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
        pipeline_create_info.basePipelineIndex = -1;

        result = vkCreateGraphicsPipelines(_device->_logical_device, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &_pipeline[chain_index]);
    }
}
