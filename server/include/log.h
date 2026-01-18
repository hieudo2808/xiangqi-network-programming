#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <time.h>

#define LOG_LEVEL_DEBUG 0
#define LOG_LEVEL_INFO 1
#define LOG_LEVEL_WARN 2
#define LOG_LEVEL_ERROR 3
#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_INFO
#endif

#define LOG_DEBUG(fmt, ...)                                                                                            \
    do {                                                                                                               \
        if (LOG_LEVEL <= LOG_LEVEL_DEBUG)                                                                              \
            fprintf(stderr, "[DEBUG] " fmt "\n", ##__VA_ARGS__);                                                       \
    } while (0)

#define LOG_INFO(fmt, ...)                                                                                             \
    do {                                                                                                               \
        if (LOG_LEVEL <= LOG_LEVEL_INFO)                                                                               \
            fprintf(stderr, "[INFO] " fmt "\n", ##__VA_ARGS__);                                                        \
    } while (0)

#define LOG_WARN(fmt, ...)                                                                                             \
    do {                                                                                                               \
        if (LOG_LEVEL <= LOG_LEVEL_WARN)                                                                               \
            fprintf(stderr, "[WARN] " fmt "\n", ##__VA_ARGS__);                                                        \
    } while (0)

#define LOG_ERROR(fmt, ...)                                                                                            \
    do {                                                                                                               \
        if (LOG_LEVEL <= LOG_LEVEL_ERROR)                                                                              \
            fprintf(stderr, "[ERROR] " fmt "\n", ##__VA_ARGS__);                                                       \
    } while (0)

#endif