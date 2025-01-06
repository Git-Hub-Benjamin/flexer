// platform.h
#ifndef PLATFORM_H
#define PLATFORM_H

#include <string.h>  // Add missing string.h header

// Platform detection
#if defined(_WIN32) || defined(_WIN64)
    #define PLATFORM_WINDOWS
#else
    #define PLATFORM_UNIX
#endif

// Safe file operations
#ifdef PLATFORM_WINDOWS
    #define _CRT_SECURE_NO_WARNINGS  // Disable deprecation warnings
    #include <stdio.h>
    
    // Safe file open macro
    #define SAFE_FOPEN(file, path, mode) \
        errno_t err = fopen_s(&file, path, mode); \
        if (err != 0) { \
            EXIT_FAIL_MSG("Failed to open file..."); \
            return NULL; \
        }
#else
    #define SAFE_FOPEN(file, path, mode) \
        file = fopen(path, mode); \
        if (file == NULL) { \
            EXIT_FAIL_MSG("Failed to open file..."); \
            return NULL; \
        }
#endif

// Size_t format specifier
#ifdef PLATFORM_WINDOWS
    #define SIZE_T_FMT "%zu"
#else
    #define SIZE_T_FMT "%lu"
#endif

#endif // PLATFORM_H