
#ifndef DEBUG_H
#define DEBUG_H

#include <time.h>
#include <cstdio>

#define NODEBUG_L 0
#define ERROR_L 1
#define WARNING_L 2
#define INFO_L 3
#define DEBUG_L 4
#define TRACE_L 5

#ifndef LOG_LEVEL
# define LOG_LEVEL INFO_L
#endif

// -------------
#ifndef COLOR_TRACE
# define COLOR_TRACE   "\e[1;35m"
#endif
#ifndef COLOR_DEBUG
# define COLOR_DEBUG   "\e[1;34m"
#endif
#ifndef COLOR_WARNING
# define COLOR_WARNING "\e[1;33m"
#endif
#ifndef COLOR_ERROR
# define COLOR_ERROR   "\e[1;31m"
#endif
#ifndef COLOR_INFO
# define COLOR_INFO   "\e[1;32m"
#endif

#define COLOR_END "\e[00m"

#if (LOG_LEVEL >= TRACE_L)
# ifdef COLOR_TRACE
#  define TRACE(str, ...) \
    std::fprintf(stdout, COLOR_TRACE "[T] - %lu -  %s:%d %s: " str COLOR_END "\n", time (NULL),  __FILE__, __LINE__, __func__,  ##__VA_ARGS__) && fflush(NULL)
# else
#  define TRACE(str, ...) \
    std::fprintf(stdout, "TRACE: %lu - " str "\n", time (NULL), ##__VA_ARGS__) && fflush(NULL)
# endif
#else
# define TRACE(...)
#endif

#if (LOG_LEVEL >= DEBUG_L)
# ifdef COLOR_DEBUG
#  define DEBUG(str, ...) \
    std::fprintf(stdout, COLOR_DEBUG "[D] %s:%d %s: " str COLOR_END "\n", __FILE__, __LINE__, __func__,  ##__VA_ARGS__)
# else
#  define DEBUG(str, ...) \
    std::fprintf(stdout, "DEBUG: " str "\n", ##__VA_ARGS__)
# endif
#else
# define DEBUG(...)
#endif

#if (LOG_LEVEL >= INFO_L)
# ifdef COLOR_INFO
#  define INFO(str, ...) \
    std::fprintf(stdout, COLOR_INFO str COLOR_END "\n", ##__VA_ARGS__)
# else
#  define INFO(str, ...) \
    std::fprintf(stdout, str "\n", ##__VA_ARGS__)
# endif
#else
# define INFO(...)
#endif

#if (LOG_LEVEL >= WARNING_L)
# ifdef COLOR_WARNING
#  define WARNING(str, ...) \
    std::fprintf(stderr, COLOR_WARNING "WARNING: " str COLOR_END "\n", ##__VA_ARGS__)
# else
#  define WARNING(str, ...) \
    std::fprintf(stderr, "WARNING: " str "\n", ##__VA_ARGS__)
# endif
#else
# define WARNING(...)
#endif

#if (LOG_LEVEL >= ERROR_L)
# ifdef COLOR_ERROR
#  define ERROR(str, ...) \
    std::fprintf(stderr, COLOR_ERROR "ERROR: " str COLOR_END "\n", ##__VA_ARGS__)
# else
#  define ERROR(str, ...) \
    std::fprintf(stderr, "ERROR: " str "\n", ##__VA_ARGS__)
# endif
#else
# define ERROR(...)
#endif

#endif
