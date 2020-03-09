//
//  debug_utils.hpp
//  vulkan-demos
//
//  Created by Rafael Sabino on 3/5/20.
//  Copyright © 2020 Rafael Sabino. All rights reserved.
//

#pragma once

#if defined(DEBUG)

#include <execinfo.h>
#include <iostream>
#include <cxxabi.h>
#include <memory>

namespace vk
{
    class debug
    {
    public:
        //based off of: https://developer.apple.com/library/archive/documentation/System/Conceptual/ManPages_iPhoneOS/man3/backtrace.3.html
        //and: https://www.variadic.xyz/2013/04/14/generating-stack-trace-on-os-x/
        static void print_callstack()
        {
            void* callstack[128] = {};

            int i, frames = backtrace(callstack, 128);
            char** strs = backtrace_symbols(callstack, frames);
            
            for (i = 0; i < frames; ++i)
            {
                char functionSymbol[1024] = {};
                char module_name[1024] = {};
                int  offset = 0;
                char addr[48] = {};
                
                // split the string, take out chunks out of stack trace
                // we are primarily interested in module, function and address
                sscanf(strs[i], "%*s %s %s %s %*s %d",
                       &module_name, &addr, &functionSymbol, &offset);
                
                int   valid_cpp_name = 0;
                //  if this is a C++ library, symbol will be demangled
                //  on success function returns 0
                //
                char* function_name = abi::__cxa_demangle(functionSymbol,
                                                         NULL, 0, &valid_cpp_name);
                
                char stack_frame[4096] = {};
                if (valid_cpp_name == 0) // success
                {
                    sprintf(stack_frame, "(%s)\t0x%s — %s + %d",
                            module_name, addr, function_name, offset);
                }
                else
                {
                    //  in the above traceback (in comments) last entry is not
                    //  from C++ binary, last frame, libdyld.dylib, is printed
                    //  from here
                    sprintf(stack_frame, "(%s)\t0x%s — %s + %d",
                            module_name, addr, function_name, offset);
                }
                
                std::cout << stack_frame << std::endl;
                if (function_name)
                {
                    free(function_name);
                }

            }
            free(strs);
        }
    };
};
#endif

