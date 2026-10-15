#pragma once

#if defined(_WIN32) || defined(_WIN64)
    #ifdef LL_EXPORT
        #define LL_API __declspec(dllexport)
    #else
        #define LL_API __declspec(dllimport)
    #endif
    #define LL_NDAPI [[nodiscard]] LL_API
#else
#include <cstdint>

using uchar = unsigned char;
using schar = signed char;
using uint = unsigned int;
using ushort = unsigned short;
    #define LL_API __attribute__((visibility("default")))
    #define LL_NDAPI [[nodiscard]] __attribute__((visibility("default")))
#endif
