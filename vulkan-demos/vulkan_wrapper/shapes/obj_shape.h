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
#include <limits>


namespace vk {
    
    template<typename RENDER_TEXTURE_TYPE, uint32_t NUM_ATTACHMENTS>
    class render_pass;

    //note: the name obj_shape comes from the fact that these objects are created by reading .obj files
    class obj_shape  : object
    {
    protected:
        obj_shape(vk::device* device){ _device = device; };
    public:
        obj_shape(device* device, const char* path);
        
        virtual void destroy() override;
        
        virtual void create();
        
        inline void set_id(uint32_t id) { _id = id; }
        inline uint32_t get_id(){
            
            return _id;
        }
        inline void bind_verteces(VkCommandBuffer& buffer, uint32_t mesh_id)
        {
            assert(_meshes.size() > mesh_id);
            _meshes[mesh_id]->bind_verteces(buffer);
        }
        
        inline void draw_indexed(VkCommandBuffer& buffer, uint32_t mesh_id)
        {
            assert(_meshes.size() > mesh_id);
            _meshes[mesh_id]->draw_indexed(buffer);
        }
        
        inline void draw(VkCommandBuffer& buffer, uint32_t mesh_id)
        {
            assert(_meshes.size() > mesh_id);
            _meshes[mesh_id]->draw(buffer);
        }
        
        inline size_t get_num_meshes(){ return _meshes.size(); }
        static const std::string _shape_resource_path;
        
//        template<typename RENDER_TEXTURE_TYPE, uint32_t NUM_ATTACHMENTS>
//        void draw(VkCommandBuffer commnad_buffer,  uint32_t swapchain_index);
        
        virtual void set_diffuse(glm::vec3 diffuse);
        
        vk::transform transform;

    protected:
        
        glm::vec3 _diffuse = glm::vec3(1.0f);
        std::vector<mesh*> _meshes;
        device* _device = nullptr;
        uint32_t _id = std::numeric_limits<uint32_t>::max();
        const char* _path = nullptr;
    };

//    template<typename RENDER_TEXTURE_TYPE, uint32_t NUM_ATTACHMENTS>
//    void obj_shape::draw(VkCommandBuffer commnad_buffer, uint32_t swapchain_index)
//    {
//        for( mesh* m : _meshes)
//        {
////            for( uint32_t subpass_id = 0; subpass_id < render_pass.get_number_of_subpasses(); ++subpass_id)
////            {
////                render_pass.get_subpass(subpass_id).begin_subpass_recording(commnad_buffer, swapchain_index );
////                if( subpass_id == 0)
//                    m->draw(commnad_buffer, render_pass, _id, swapchain_index);
//
//            //}
//        }
//    }

}
