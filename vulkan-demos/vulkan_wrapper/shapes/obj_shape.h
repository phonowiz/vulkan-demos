//
//  shape.hpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 8/8/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#pragma once


#include <string>
#include <vector>

#include "mesh.h"
#include "../core/object.h"
#include "transform.h"


namespace vk {
    
    template<typename RENDER_TEXTURE_TYPE, uint32_t NUM_ATTACHMENTS>
    class graphics_pipeline;

    //note: the name obj_shape comes from the fact that these objects are created by reading .obj files
    class obj_shape  : object
    {
    protected:
        obj_shape(vk::device* device){ _device = device; };
    public:
        obj_shape(device* device, const char* path);
        
        virtual void destroy() override;
        
        virtual void create();
        static const std::string _shape_resource_path;
        
        template<typename RENDER_TEXTURE_TYPE, uint32_t NUM_ATTACHMENTS>
        void draw(VkCommandBuffer commnad_buffer, vk::graphics_pipeline<RENDER_TEXTURE_TYPE, NUM_ATTACHMENTS>& pipeline, uint32_t obj_id, uint32_t swapchain_index);
        
        virtual void set_diffuse(glm::vec3 diffuse);
        
        vk::transform transform;

    protected:
        
        glm::vec3 _diffuse = glm::vec3(1.0f);
        std::vector<mesh*> _meshes;
        device* _device = nullptr;
        const char* _path = nullptr;
    };

    template<typename RENDER_TEXTURE_TYPE, uint32_t NUM_ATTACHMENTS>
    void obj_shape::draw(VkCommandBuffer commnad_buffer, vk::graphics_pipeline<RENDER_TEXTURE_TYPE, NUM_ATTACHMENTS>& pipeline, uint32_t obj_id, uint32_t swapchain_index)
    {
        for( mesh* m : _meshes)
        {
            m->draw(commnad_buffer, pipeline, obj_id, swapchain_index);
        }
    }

}
