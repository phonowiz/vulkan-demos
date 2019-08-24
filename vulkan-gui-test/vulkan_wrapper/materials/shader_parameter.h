//
//  ShaderParameter.hpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 2/14/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#pragma once


#include "glm/gtc/type_ptr.hpp"
#include "glm/glm.hpp"
#include "texture_2d.h"
#include "texture_3d.h"

#include <map>
#include "ordered_map.h"

/// <summary> Represents a setting for a material that can be used for a shader </summary>

namespace vk
{
    class shader_parameter
    {
        
    private:
        static constexpr size_t MAX_UNIFORM_BUFFER_SIZE = 512;
        struct values_array
        {
            char* memory[MAX_UNIFORM_BUFFER_SIZE];
            size_t num_elements = 0;
        };
        
    public:
        enum class Type
        {
            MAT4,
            VEC4,
            VEC3,
            VEC2,
            FLOAT,
            INT,
            BOOLEAN,
            UINT,
            SAMPLER_2D,
            SAMPLER_3D,
            VEC4_ARRAY,
            NONE
            
        };
        
    private:
        
        union setting_value
        {
            glm::vec4 vector4;
            glm::vec3 vector3;
            glm::vec2 vector2;
            float   float_value;
            int     intValue;
            unsigned int    uintValue;
            texture_2d*   sampler2D;
            texture_3d*   sampler3D;
            
            glm::mat4 mat4;
            bool     boolean;
            
            values_array buffer;
            
            setting_value()
            {
                buffer = {};
            }
            
        };
        
        setting_value value;
        Type type;
        const char* name = nullptr;
        
    public:
        
        inline Type get_type(){return type;}
        inline setting_value* get_stored_value_memory(){ return &value; }
        
        using shader_params_group = ordered_map<const char* ,shader_parameter>;
        using KeyValue = std::pair<const char*, shader_parameter> ;
        
        shader_parameter():value(),type(Type::NONE)
        {}
        
        //the size returned here should be big enough ( safe enough) to store whatever bytes we pass it.
        inline size_t aligned_size(size_t alignment, size_t bytes)
        {
            assert((alignment > 0) && (alignment & ~(alignment -1)) && "alignment must be greater than 0 && power of 2");
            return (alignment -1 ) + bytes;
        }
        
        
        inline size_t get_type_size()
        {
            switch( type )
            {
                case Type::INT:
                    return sizeof(int);
                case Type::FLOAT:
                    return sizeof(float);
                case Type::MAT4:
                    return sizeof(glm::mat4);
                case Type::VEC3:
                    return sizeof(glm::vec3);
                case Type::VEC4:
                    return sizeof(glm::vec4);
                case Type::VEC2:
                    return sizeof(glm::vec2);
                case Type::UINT:
                    return sizeof(unsigned int);
                case Type::BOOLEAN:
                    return sizeof(bool);
                case Type::SAMPLER_2D:
                    return sizeof(texture_2d);
                case Type::SAMPLER_3D:
                    return sizeof(texture_3d);
                case Type::VEC4_ARRAY:
                    return value.buffer.num_elements * sizeof(glm::vec4);
                case Type::NONE:
                    assert(0);
                    break;
                    
            }
        }
        //note: this function follows std140 alignment rules, in your glsl shader, make sure to specify std140 as your choice
        //for memory layout.
        inline size_t get_std140_alignment()
        {
            switch( type )
            {
                case Type::INT:
                    return sizeof(int);
                case Type::FLOAT:
                    return sizeof(float);
                case Type::MAT4:
                case Type::VEC3:
                case Type::VEC4:
                    return 4 * sizeof(float);
                case Type::VEC2:
                    return 2 * sizeof(float);
                case Type::UINT:
                    return sizeof(unsigned int);
                case Type::BOOLEAN:
                    return sizeof(bool);
                case Type::SAMPLER_2D:
                    return sizeof(texture_2d);
                case Type::SAMPLER_3D:
                    return sizeof(texture_3d);
                case Type::VEC4_ARRAY:
                    return 4 * sizeof(float);
                case Type::NONE:
                    assert(0);
                    break;
                
            }
        }

        inline size_t get_std140_aligned_size_in_bytes()
        {
            size_t result = 0;
            switch( type )
            {
                case Type::INT:
                    result = aligned_size(get_std140_alignment(), sizeof(int));
                    break;
                case Type::FLOAT:
                    result =  aligned_size(get_std140_alignment(), sizeof(float));
                    break;
                case Type::BOOLEAN:
                    result =  aligned_size(get_std140_alignment(), sizeof( bool));
                    break;
                case Type::UINT:
                    result = aligned_size(get_std140_alignment(), sizeof( unsigned int));
                    break;
                case Type::MAT4:
                    result = aligned_size(get_std140_alignment(), sizeof(glm::mat4));
                    break;
                case Type::VEC2:
                    result = aligned_size(get_std140_alignment(), sizeof( glm::vec2));
                    break;
                case Type::VEC3:
                    result = aligned_size(get_std140_alignment(), sizeof( glm::vec3));
                    break;
                case Type::VEC4:
                    result =  aligned_size(get_std140_alignment(), sizeof(glm::vec4));
                    break;
                case Type::SAMPLER_2D:
                    assert(0);
                    result = sizeof (texture_2d);
                    break;
                case Type::SAMPLER_3D:
                    assert(0);
                    result = sizeof (texture_3d);
                    break;
                case Type::VEC4_ARRAY:
                {
                    size_t vec4_size = aligned_size(get_std140_alignment(), sizeof(glm::vec4));
                    result = value.buffer.num_elements * vec4_size;
                    break;
                }
                case Type::NONE:
                {
                    assert(0 && "this case should never happen");
                    result = 0;
                    break;
                }
            };
            
            return result;
        }
        
        void* write_to_buffer(void* p, size_t& mem_size)
        {
            char* ptr = nullptr;
            if(type == Type::VEC4_ARRAY)
            {
                glm::vec4* vecs = reinterpret_cast<glm::vec4*>(value.buffer.memory);
                for(size_t i = 0; i < value.buffer.num_elements; ++i)
                {
                    void* result = std::align( get_std140_alignment(), sizeof(glm::vec4), p, mem_size);
                    assert(result);
                    std::memcpy(p, &vecs[i], sizeof(glm::vec4));
                    mem_size -= sizeof(glm::vec4);
                    ptr = static_cast<char*>(p);
                    ptr+= sizeof(glm::vec4);
                    p = reinterpret_cast<void*>(ptr);
                }
            }
            else
            {

                assert(p != nullptr);

                void* result = std::align( get_std140_alignment(),get_type_size(), p, mem_size);
                assert(result);
                assert(mem_size >= get_type_size());
                mem_size -= get_type_size();
                std::memcpy(p, get_stored_value_memory(), get_type_size());
                ptr = static_cast<char*>(p);
                ptr+= get_type_size();
                p = reinterpret_cast<void*>(ptr);
                *ptr += get_type_size();
            }

            
            return reinterpret_cast<void*>(ptr);
        }
        
        inline texture_2d* get_texture_2d()
        {
            assert(type == Type::SAMPLER_2D);
            return value.sampler2D;
        }
        
        inline texture_3d* get_texture_3d()
        {
            assert(type == Type::SAMPLER_3D);
            return value.sampler3D;
        }
        
        inline image* get_image()
        {
            assert(type == Type::SAMPLER_2D || type == Type::SAMPLER_3D);
            
            if(type == Type::SAMPLER_2D)
            {
                return static_cast<image*>(value.sampler2D);
            }
            
            return static_cast<image*>(value.sampler3D);
        }
        
        inline void set_vectors_array(glm::vec4* vecs, size_t num_vectors)
        {
            assert( type == Type::NONE || type == Type::VEC4_ARRAY);
            type = Type::VEC4_ARRAY;
            value.buffer.num_elements = num_vectors;
            void* data = reinterpret_cast<void*>(value.buffer.memory);
            assert((value.buffer.num_elements * sizeof(glm::vec4)) < MAX_UNIFORM_BUFFER_SIZE);
            std::memcpy(data, &vecs[0], num_vectors * sizeof(glm::vec4));
        }
        
        inline shader_parameter& operator=(const glm::mat4 &value)
        {
            assert( type == Type::NONE || type == Type::MAT4);
            type = Type::MAT4;
            this->value.mat4 = value;
            
            return *this;
        }
        
        inline shader_parameter& operator=(const float &value)
        {
            assert( type == Type::NONE || type == Type::FLOAT);
            type = Type::FLOAT;
            this->value.float_value = value;
            
            return *this;
        }
        
        inline shader_parameter& operator=(const glm::vec4 &value)
        {
            assert( type == Type::NONE || type == Type::VEC4);
            type = Type::VEC4;
            this->value.vector4 = value;
            
            return *this;
        }
        
        inline shader_parameter& operator=(const glm::vec3 &value)
        {
            assert( type == Type::NONE || type == Type::VEC3);
            type = Type::VEC3;
            this->value.vector3 = value;
            
            return *this;
        }
        
        inline shader_parameter& operator=(const glm::vec2 &value)
        {
            assert( type == Type::NONE || type == Type::VEC2);
            type = Type::VEC2;
            this->value.vector2 = value;
            
            return *this;
        }
        
        inline shader_parameter& operator=(const int &value)
        {
            assert( type == Type::NONE || type == Type::INT);
            type = Type::INT;
            this->value.intValue = value;
            
            return *this;
        }
        
        inline shader_parameter& operator=(const unsigned int &value)
        {
            assert( type == Type::NONE || type == Type::UINT);
            type = Type::UINT;
            this->value.intValue = value;
            
            return *this;
        }
        
        inline shader_parameter& operator=(texture_2d* sampler)
        {
            assert( type == Type::NONE || type == Type::SAMPLER_2D);
            type = Type::SAMPLER_2D;
            this->value.sampler2D = sampler;
    
            return *this;
        }
        
        inline shader_parameter& operator=(texture_3d* sampler)
        {
            assert( type == Type::NONE || type == Type::SAMPLER_3D);
            type = Type::SAMPLER_3D;
            this->value.sampler3D = sampler;
            
            return *this;
        }
        
        inline shader_parameter& operator=(image* sampler)
        {
            assert( type == Type::NONE || type == Type::SAMPLER_3D || type == Type::SAMPLER_2D);
            
            if(sampler->get_instance_type() == texture_3d::get_class_type())
            {
                type = Type::SAMPLER_3D;
                value.sampler3D = static_cast<texture_3d*>( sampler );
            }
            else if( sampler->get_instance_type() == texture_2d::get_class_type())
            {
                type = Type::SAMPLER_2D;
                value.sampler2D = static_cast<texture_2d*>( sampler );
            }
            else
            {
                assert(0 && "image type not recognized");
            }
            
            return *this;
        }
        
        inline shader_parameter& operator=(const bool value)
        {
            type = Type::BOOLEAN;
            this->value.boolean = value;
            return *this;
        }
        
        shader_parameter(glm::mat4 _value)
        {
            value.mat4 = _value;
            type = Type::MAT4;
        }
        
        shader_parameter(glm::vec4 _value)
        {
            value.vector4 = _value;
            type = Type::VEC4;
        }
        
        shader_parameter(glm::vec3 _value)
        {
            value.vector3 = _value;
            type = Type::VEC3;
        }
        
        shader_parameter(glm::vec2 _value)
        {
            value.vector2 = _value;
            type = Type::VEC2;
        }
        
        shader_parameter(int _value)
        {
            value.intValue = _value;
            type = Type::INT;
        }
        shader_parameter(unsigned int _value)
        {
            value.uintValue = _value;
            type = Type::UINT;
        }
        
        shader_parameter(float _value)
        {
            value.float_value= _value;
            type = Type::FLOAT;
        }
        
        shader_parameter(texture_2d* _value)
        {
            value.sampler2D = _value;
            type = Type::SAMPLER_2D;
        }
    
        shader_parameter(texture_3d* _value)
        {
            value.sampler3D = _value;
            type = Type::SAMPLER_3D;
        }
        
    private:
        
    };

}
