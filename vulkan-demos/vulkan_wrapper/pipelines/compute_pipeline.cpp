//
//  compute_pipeline.cpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 6/29/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#include "compute_pipeline.h"

using namespace vk;

void compute_pipeline::create()
{
    assert(_pipeline == VK_NULL_HANDLE);
    
    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
    pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_create_info.pNext = nullptr;
    pipeline_layout_create_info.flags = 0;
    pipeline_layout_create_info.setLayoutCount = material->descriptor_set_present() ? 1 : 0;
    pipeline_layout_create_info.pSetLayouts = material->get_descriptor_set_layout();
    pipeline_layout_create_info.pushConstantRangeCount = 0;
    pipeline_layout_create_info.pPushConstantRanges = nullptr;
    
    VkResult result = vkCreatePipelineLayout(_device->_logical_device, &pipeline_layout_create_info, nullptr, &_pipeline_layout);
    ASSERT_VULKAN(result);
    
    VkComputePipelineCreateInfo compute_pipeline_create_info {};
    compute_pipeline_create_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    compute_pipeline_create_info.layout = _pipeline_layout;
    compute_pipeline_create_info.flags = 0;
    compute_pipeline_create_info.stage = *material->get_shader_stages();
    
    result = vkCreateComputePipelines(_device->_logical_device,VK_NULL_HANDLE, 1, &compute_pipeline_create_info, nullptr, &_pipeline);
    ASSERT_VULKAN(result);
    
    material->commit_parameters_to_gpu();
}

void compute_pipeline::record_dispatch_commands( VkCommandBuffer&  command_buffer,
                                                 uint32_t local_groups_in_x, uint32_t local_groups_in_y, uint32_t local_groups_in_z)
{
    material->commit_parameters_to_gpu();
    
    if(_pipeline == VK_NULL_HANDLE)
    {
        //last minute creating the pipeline to give client a change to set parameters
        create();
    }
    
    VkCommandBufferBeginInfo command_buffer_begin_info {};
    command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_begin_info.pInheritanceInfo = nullptr;
    
    VkResult result = vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info);
    ASSERT_VULKAN(result);
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, _pipeline);

    _on_begin();
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, _pipeline_layout, 0, 1, material->get_descriptor_set(), 0, 0);
    
    vkCmdDispatch(command_buffer, local_groups_in_x, local_groups_in_y, local_groups_in_z);
    
    result = vkEndCommandBuffer(command_buffer);
    ASSERT_VULKAN(result);
}
