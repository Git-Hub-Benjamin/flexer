// platform.h
#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdio.h>
#include <string.h>
#include <errno.h>

// Platform detection
#if defined(_WIN32) || defined(_WIN64)
    #define PLATFORM_WINDOWS
#else
    #define PLATFORM_UNIX
#endif

// Safe file operations
#ifdef PLATFORM_WINDOWS
    #include <stdlib.h>
    #define SAFE_FOPEN(file, path, mode) \
        errno_t err = fopen_s(&file, path, mode); \
        if (err != 0 || file == NULL) { \
            EXIT_FAIL_MSG("Failed to open file..."); \
            return NULL; \
        }

    // Safe string copy for Windows
    #define SAFE_STRCPY(dest, destSize, src, srcSize) \
        strncpy_s(dest, destSize, src, srcSize)

#else
    // Unix/Linux version
    #define SAFE_FOPEN(file, path, mode) \
        file = fopen(path, mode); \
        if (file == NULL) { \
            EXIT_FAIL_MSG("Failed to open file..."); \
            return NULL; \
        }

    // Safe string copy for Unix/Linux
    #define SAFE_STRCPY(dest, destSize, src, srcSize) \
        strncpy(dest, src, destSize - 1); \
        dest[destSize - 1] = '\0'
#endif

// Size_t format specifier
#ifdef PLATFORM_WINDOWS
    #define SIZE_T_FMT "%zu"
#else
    #define SIZE_T_FMT "%zu"  // Modern Unix systems also use %zu
#endif

#endif // PLATFORM_H