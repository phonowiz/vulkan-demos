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
    for( int i =0; i < glfw_swapchain::NUM_SWAPCHAIN_IMAGES; ++i)
    {
        VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
        pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipeline_layout_create_info.pNext = nullptr;
        pipeline_layout_create_info.flags = 0;
        pipeline_layout_create_info.setLayoutCount = _material[i]->descriptor_set_present() ? 1 : 0;
        pipeline_layout_create_info.pSetLayouts = _material[i]->get_descriptor_set_layout();
        pipeline_layout_create_info.pushConstantRangeCount = 0;
        pipeline_layout_create_info.pPushConstantRanges = nullptr;
        
        VkResult result = vkCreatePipelineLayout(_device->_logical_device, &pipeline_layout_create_info, nullptr, &_pipeline_layout[i]);
        ASSERT_VULKAN(result);
        
        VkComputePipelineCreateInfo compute_pipeline_create_info {};
        compute_pipeline_create_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        compute_pipeline_create_info.layout = _pipeline_layout[i];
        compute_pipeline_create_info.flags = 0;
        compute_pipeline_create_info.stage = *_material[i]->get_shader_stages();
        
        result = vkCreateComputePipelines(_device->_logical_device,VK_NULL_HANDLE, 1, &compute_pipeline_create_info, nullptr, &_pipeline[i]);
        ASSERT_VULKAN(result);
    }

}

void compute_pipeline::record_dispatch_commands( VkCommandBuffer&  command_buffer,
                                                 uint32_t local_groups_in_x, uint32_t local_groups_in_y, uint32_t local_groups_in_z, uint32_t swapchain_index)
{
    //make sure to have all shader parameters ready for consumption, this is necessary to create the pipeline as well
    _material[swapchain_index]->commit_parameters_to_gpu();
    
    if(!is_initialized())
    {
        //last minute creating the pipeline to give client a chance to specify all parameters necessary for shader to run
        create();
    }
    
    VkCommandBufferBeginInfo command_buffer_begin_info {};
    command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_begin_info.pInheritanceInfo = nullptr;
    
    VkResult result = vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info);
    ASSERT_VULKAN(result);
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, _pipeline[swapchain_index]);

    _on_begin();
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, _pipeline_layout[swapchain_index], 0, 1, _material[swapchain_index]->get_descriptor_set(), 0, 0);
    
    vkCmdDispatch(command_buffer, local_groups_in_x, local_groups_in_y, local_groups_in_z);
    
    result = vkEndCommandBuffer(command_buffer);
    ASSERT_VULKAN(result);
}
