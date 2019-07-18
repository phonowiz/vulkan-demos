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
    pipeline_layout_create_info.setLayoutCount = _material->descriptor_set_present() ? 1 : 0;
    pipeline_layout_create_info.pSetLayouts = _material->get_descriptor_set_layout();
    pipeline_layout_create_info.pushConstantRangeCount = 0;
    pipeline_layout_create_info.pPushConstantRanges = nullptr;
    
    VkResult result = vkCreatePipelineLayout(_device->_logical_device, &pipeline_layout_create_info, nullptr, &_pipeline_layout);
    ASSERT_VULKAN(result);
    
    VkComputePipelineCreateInfo compute_pipeline_create_info {};
    compute_pipeline_create_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    compute_pipeline_create_info.layout = _pipeline_layout;
    compute_pipeline_create_info.flags = 0;
    compute_pipeline_create_info.stage = *_material->get_shader_stages();
    
    result = vkCreateComputePipelines(_device->_logical_device,VK_NULL_HANDLE, 1, &compute_pipeline_create_info, nullptr, &_pipeline);
    ASSERT_VULKAN(result);
    
    _material->commit_parameters_to_gpu();
    
    
}
