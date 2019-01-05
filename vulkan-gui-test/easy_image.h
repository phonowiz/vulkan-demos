//
//  EasyImage.h
//  vulkan-gui-test
//
//  Created by Rafael Sabino on 1/4/19.
//  Copyright © 2019 Rafael Sabino. All rights reserved.
//

#pragma once
#include "vulkan_utils.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

class EasyImage
{
    
private:
    int width;
    int height;
    int channels;
    stbi_uc *ppixels;
    
    bool loaded = false;
    
public:
    
    EasyImage():
    loaded(false),
    ppixels(nullptr),
    width(0),
    height(0),
    channels(0)
    {
    }
    
    EasyImage(const char* path)
    {
        load(path);
    }
    
    ~EasyImage()
    {
        destroy();
    }
    
    void load( const char* path)
    {
        assert(loaded !=true);
        ppixels = stbi_load(path, &width, &height, &channels, STBI_rgb_alpha);
        
        assert(ppixels != nullptr);
        loaded = true;
    }
    
    void destroy()
    {
        if(loaded)
        {
            stbi_image_free(ppixels);
            loaded = false;
        }
    }
    
    int getWidth()
    {
        assert(loaded);
        return width;
    }
    
    int getHeight()
    {
        assert(loaded);
        return height;
    }
    
    int getChannels()
    {
        assert(loaded);
        assert(channels == 4);
        return channels;
    }
    
    int getSizeInBytes()
    {
        assert(loaded);
        return getWidth() * getHeight() * getChannels();
    }
    
    stbi_uc * getRaw()
    {
        assert(loaded);
        return ppixels;
    }

};
