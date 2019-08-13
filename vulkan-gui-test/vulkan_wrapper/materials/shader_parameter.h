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

#include "tsl/ordered_map.h"

/// <summary> Represents a setting for a material that can be used for a shader </summary>

namespace vk
{
    //TODO: LET'S TRY IMPLEMENTING THIS WITH TEMPLATE CLASSES
    class shader_parameter
    {
        
    private:
        static constexpr size_t MAX_UNIFORM_BUFFER_SIZE = 1024;
        struct uniform_buffer
        {
            char* memory[MAX_UNIFORM_BUFFER_SIZE];
            size_t size = 0;
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
            //        POINT_LIGHT,
            UNIFORM_BUFFER,
            NONE
            
        };
        
    private:
        

        

        union setting_value
        {
            glm::vec4 vector4;
            glm::vec3 vector3;
            glm::vec2 vector2;
            float   floatValue;
            int     intValue;
            unsigned int    uintValue;
            texture_2d*   sampler2D;
            texture_3d*   sampler3D;
            
            glm::mat4 mat4;
            //        PointLight pointLight;
            bool     boolean;
            
            uniform_buffer buffer;
            
            setting_value()
            {
                mat4 = glm::mat4(0);
            }
            
        };
        
        setting_value value;
        Type type;
        const char* name = nullptr;
        
    public:
        
        inline Type get_type(){return type;}
        inline setting_value* get_stored_value_memory(){ return &value; }
        
        using shader_params_group =  tsl::ordered_map<const char* ,shader_parameter>;
        using KeyValue = std::pair<const char*, shader_parameter> ;
        
        shader_parameter():value(),type(Type::NONE)
        {
            
        }
        inline size_t get_size_in_bytes()
        {
            switch( type )
            {
                case Type::INT:
                    return sizeof(int);
                    break;
                case Type::FLOAT:
                    return sizeof(float);
                    break;
                case Type::MAT4:
                    return sizeof(glm::mat4);
                    break;
                case Type::VEC2:
                    return sizeof( glm::vec2);
                    break;
                case Type::VEC3:
                    return sizeof( glm::vec3);
                    break;
                case Type::VEC4:
                    return sizeof( glm::vec4);
                    break;
                case Type::BOOLEAN:
                    return sizeof( bool);
                    break;
                case Type::UINT:
                    return sizeof( unsigned int);
                    break;
                case Type::SAMPLER_2D:
                    return sizeof (texture_2d);
                    break;
                case Type::SAMPLER_3D:
                    return sizeof (texture_3d);
                    break;
                case Type::UNIFORM_BUFFER:
                    return value.buffer.size;
                    break;
                case Type::NONE:
                    assert(0);
                    break;
            };
            
            return 0;
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
        
        inline void set_uniform_buffer(void* memory, size_t size_in_bytes )
        {
            type = Type::UNIFORM_BUFFER;
            value.buffer.size = size_in_bytes;
            assert(size_in_bytes < MAX_UNIFORM_BUFFER_SIZE);
            std::memcpy(static_cast<void*>(&value.buffer.memory[0]), memory, size_in_bytes);
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
            this->value.floatValue = value;
            
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
            value.floatValue= _value;
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
