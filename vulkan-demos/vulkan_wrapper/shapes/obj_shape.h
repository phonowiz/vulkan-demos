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
    
    template< uint32_t NUM_ATTACHMENTS>
    class render_pass;

    //note: the name obj_shape comes from the fact that these objects are created by reading .obj files
    class obj_shape  : public object
    {
    protected:
        obj_shape(vk::device* device){ _device = device; };
    public:
        
        obj_shape(){}
        obj_shape(device* device, const char* path);
        
        void set_device(device* dev)
        {
            _device = dev;
        }
        virtual void destroy() override;
        
        virtual void create();
        
        inline void set_id(uint32_t id)
        {
            _id = id;
        }
        
        inline uint32_t get_id()
        {
            return _id;
        }
        
        inline void bind_verteces(VkCommandBuffer& buffer, uint32_t mesh_id)
        {
            assert(_meshes.size() > mesh_id);
            _meshes[mesh_id]->bind_verteces(buffer);
        }
        
        inline void draw_indexed(VkCommandBuffer& buffer, uint32_t mesh_id, uint32_t instance_count)
        {
            assert(_meshes.size() > mesh_id);
            _meshes[mesh_id]->draw_indexed(buffer, instance_count);
        }
        
        inline void draw(VkCommandBuffer& buffer, uint32_t mesh_id)
        {
            assert(_meshes.size() > mesh_id);
            _meshes[mesh_id]->draw(buffer);
        }
        
        inline size_t get_num_meshes(){ return _meshes.size(); }
        static const std::string _shape_resource_path;
        
        virtual void set_diffuse(glm::vec3 diffuse);
        
        vk::transform transform;

    protected:
        
        glm::vec3 _diffuse = glm::vec3(1.0f);
        std::vector<mesh*> _meshes;
        device* _device = nullptr;
        uint32_t _id = std::numeric_limits<uint32_t>::max();
        const char* _path = nullptr;
    };
}
