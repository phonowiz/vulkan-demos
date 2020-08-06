//
//  render_pass.cpp
//  vulkan-demos
//
//  Created by Rafael Sabino on 7/16/20.
//  Copyright © 2020 Rafael Sabino. All rights reserved.
//


template<uint32_t NUM_ATTACHMENTS>
 void render_pass< NUM_ATTACHMENTS>::record_draw_commands(VkCommandBuffer& buffer, uint32_t swapchain_id, uint32_t instance_count)
 {
     EA_ASSERT_MSG(_num_objects != 0, "you must have objects to render in a subpass");
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
                     _shapes[obj_id]->draw_indexed(buffer, mesh_id, instance_count);
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
     EA_ASSERT_MSG(_attachment_group.size() <= MAX_NUMBER_OF_ATTACHMENTS, "maximum number of attachments has been exceeded");
     EA_ASSERT_MSG(_attachment_group.size() != 0, "attachment group size cannot be 0.  You need to add resource_sets to attachment groups");
     EA_ASSERT_MSG(_dimensions.x != 0 && _dimensions.y !=0, "attachment dimensions cannot be zero" );
     EA_ASSERT_MSG(_device != nullptr, "vk::device has not been assigned to this render pass");
     
     //_attachment_group.init(swapchain_id);

     eastl::array<VkAttachmentDescription, MAX_NUMBER_OF_ATTACHMENTS> attachment_descriptions {};
     //an excellent explanation of what the heck are these attachment references:
     //https://stackoverflow.com/questions/49652207/what-is-the-purpose-of-vkattachmentreference
     
     //here is article about subpasses and input attachments and how they are all tied togethere
     //https://www.saschawillems.de/blog/2018/07/19/vulkan-input-attachments-and-sub-passes/
     uint32_t attachment_id = 0;
     EA_ASSERT(_attachment_group[attachment_id].size() == glfw_swapchain::NUM_SWAPCHAIN_IMAGES);
     VkAttachmentReference depth_reference {};
     
     bool multisampling = false;
     int32_t depth_attachment_id = -1;
     for(int i =0 ; i < _attachment_group.size(); ++i)
     {
         //note: resolve attachments always have a sample count of one in this code
         multisampling = !multisampling ?  _attachment_group.is_multisample_attachment(i) : multisampling;
         EA_ASSERT_MSG(_attachment_group.size() != 0, "you need at least one attachment in render pass");
         EA_ASSERT_MSG(_attachment_group[i][swapchain_id]->is_initialized(), "call init on all your attachment group textures");
         _attachment_group[i][swapchain_id]->set_device(_device);
         EA_ASSERT_MSG(_dimensions.x == _attachment_group[i][swapchain_id]->get_width(), "attachments must have the same width ");
         EA_ASSERT_MSG(_dimensions.y == _attachment_group[i][swapchain_id]->get_height(), "attachments must have the same height");
         
         if(_attachment_group[i][0]->get_instance_type() == depth_texture::get_class_type() && is_depth_enabled())
         {
             resource_set<image*>& depths =  get_depth_textures();
             depth_texture* t = static_cast<depth_texture*>( depths[swapchain_id]);
             attachment_descriptions[attachment_id] =  t->get_depth_attachment();
             depth_reference.attachment = attachment_id;
             depth_reference.layout = static_cast<VkImageLayout>(depths[swapchain_id]->get_usage_layout(vk::usage_type::STORAGE_IMAGE));
             ++attachment_id;
             depth_attachment_id = (int32_t)attachment_id;
         }
         else
         {
             attachment_descriptions[attachment_id].samples = _attachment_group.is_multisample_attachment(i) ? _device->get_max_usable_sample_count() : VK_SAMPLE_COUNT_1_BIT;
             attachment_descriptions[attachment_id].loadOp =  _attachment_group.should_clear(i) ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;

             attachment_descriptions[attachment_id].storeOp = _attachment_group.should_store(i) ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
             attachment_descriptions[attachment_id].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
             attachment_descriptions[attachment_id].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
             
             attachment_descriptions[attachment_id].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
             attachment_descriptions[attachment_id].finalLayout = static_cast<VkImageLayout>(_attachment_group[i][swapchain_id]->get_original_layout());
             attachment_descriptions[attachment_id].format = static_cast<VkFormat>(_attachment_group[i][swapchain_id]->get_format());
             attachment_id++;
         }
     }
     
     if(depth_attachment_id != -1)
     {
         attachment_descriptions[depth_attachment_id].samples = multisampling ? _device->get_max_usable_sample_count() : VK_SAMPLE_COUNT_1_BIT;
     }

     EA_ASSERT(attachment_id != 0);
     
     eastl::array<VkSubpassDescription, MAX_SUBPASSES> subpass {};
     
     EA_ASSERT_MSG(_subpasses[0].is_active(),"You need at least one subpass for rendering to occur");
     int subpass_id = 0;
     
     eastl::array<VkSubpassDependency,MAX_SUBPASSES> dependencies {};
     
     dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
     dependencies[0].dstSubpass = 0;
     dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
     dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
     dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
     dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
     dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
     

     
     while(_subpasses[subpass_id].is_active())
     {
         _subpasses[subpass_id].init();
         subpass[subpass_id] = _subpasses[subpass_id].get_subpass_description();
         
         //an excellent explanation of what the heck are these attachment references:
         //https://stackoverflow.com/questions/49652207/what-is-the-purpose-of-vkattachmentreference
         
          if(_subpasses[subpass_id].get_depth_enable())
         {
             EA_ASSERT_MSG( _subpasses[subpass_id].is_depth_an_input() != true, "depth cannot be both an input an output in subpass, call subpass.set_depth_enable");
             subpass[subpass_id].pDepthStencilAttachment = &depth_reference;
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
     
     EA_ASSERT(_attachment_group.size() < MAX_NUMBER_OF_ATTACHMENTS);
     uint32_t num_views = 0;
     
     //add all num views for this swapchain id
     for( int i = 0; i < _attachment_group.size(); ++i)
     {
         if(_attachment_group[i][0]->get_instance_type() == depth_texture::get_class_type() && is_depth_enabled())
         {
             resource_set<image*>& depths =  get_depth_textures();
             EA_ASSERT(depths[swapchain_id]->_image_view != VK_NULL_HANDLE);
             attachment_views[num_views++]  = depths[swapchain_id]->_image_view;
         }
         else
         {
             EA_ASSERT(_attachment_group[i][swapchain_id]->is_initialized());
             EA_ASSERT(_attachment_group[i][swapchain_id] != nullptr);
             EA_ASSERT(_attachment_group[i][swapchain_id]->_image_view != VK_NULL_HANDLE && "did you initialize this image?");
             attachment_views[num_views++] = _attachment_group[i][swapchain_id]->_image_view;
         }
     }
     
     VkFramebufferCreateInfo framebuffer_create_info {};
     framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
     framebuffer_create_info.pNext = nullptr;
     framebuffer_create_info.flags = num_views == 0 ? VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT : 0;
     framebuffer_create_info.renderPass = _vk_render_passes[swapchain_id];
     framebuffer_create_info.attachmentCount = num_views;
     framebuffer_create_info.pAttachments = attachment_views.data();
     //there is an assumption that all attachments are the same width and height, I put some
     //asserts before which will check if this is true
     framebuffer_create_info.width = _dimensions.x;
     framebuffer_create_info.height = _dimensions.y;
     framebuffer_create_info.layers = 1;
     if(num_views == 1)
     {
         framebuffer_create_info.layers = _attachment_group[0][swapchain_id]->get_layer_count();
     }

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
