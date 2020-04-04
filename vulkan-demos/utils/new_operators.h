//
//  new_operators.hpp
//  vulkan-demos
//
//  Created by Rafael Sabino on 4/11/20.
//  Copyright © 2020 Rafael Sabino. All rights reserved.
//

#pragma once

void* operator new[](size_t size, const char* /*pName*/, int /*flags*/, unsigned /*debugFlags*/, const char* /*file*/, int /*line*/)
{
    return operator new(size);
}

void* operator new[](size_t size, size_t /*alignment*/, size_t /*alignmentOffset*/, const char* /*pName*/,
                        int /*flags*/, unsigned /*debugFlags*/, const char* /*file*/, int /*line*/)
{
    return operator new(size);
}

void operator delete[] (void* ptr) EA_THROW_SPEC_DELETE_NONE()
{
    operator delete(ptr);
}

