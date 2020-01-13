//
//  graphics_pipeline.cpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 3/22/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#include "graphics_pipeline.h"
#include "vertex.h"


using namespace vk;


void graphics_pipeline::init_blend_attachments()
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
