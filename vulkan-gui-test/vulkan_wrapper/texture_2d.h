//
//  texture.hpp
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 2/10/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#pragma once

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "resource.h"
#include "image.h"

#include "stb_image.h"
namespace vk
{
    class texture_2d : public image
    {
    public:
    
        texture_2d(){};
        texture_2d(device* device, uint32_t width, uint32_t height);
        texture_2d(device* device,const char* path = nullptr);
        
        enum class Formats
        {
            RGBA = VK_FORMAT_R8G8B8A8_UNORM
        };

        virtual void create( uint32_t width, uint32_t height) override;
        virtual void destroy() override;
        
        stbi_uc * getRaw()
        {
            return _ppixels;
        }
        
        VkSampler getSampler()
        {
            return _sampler;
        }
        
        int getChannels()
        {
            assert(_channels == 4);
            return _channels;
        }
        
        int getSizeInBytes()
        {
            return get_width() * get_height() * getChannels();
        }
        
        void load();
        void createSampler();
        void setSampler(device* device){ }
        
        static const std::string textureResourcePath;
    private:
        
        bool _loaded = false;
        bool _uploaded = false;
        
        stbi_uc *_ppixels = nullptr;
        VkSampler _sampler = VK_NULL_HANDLE;
        std::string _path;
        
        //TODO: we only support 4 channels at the moment
        uint32_t _channels = 4;
        //TODO: we only support RGBA
        texture_2d::Formats _format = texture_2d::Formats::RGBA;
    };
}

