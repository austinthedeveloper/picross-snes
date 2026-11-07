#ifndef TEST_SUPPORT_H
#define TEST_SUPPORT_H

#include <stdio.h>
#include <stdlib.h>

static int s_failures;

#define CHECK(cond) do { \
    if (!(cond)) { \
        fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #cond); \
        s_failures++; \
    } \
} while (0)

#define CHECK_EQ(a, b) do { \
    long _a = (long)(a), _b = (long)(b); \
    if (_a != _b) { \
        fprintf(stderr, "%s:%d: %s == %ld, expected %ld\n", __FILE__, __LINE__, #a, _a, _b); \
        s_failures++; \
    } \
} while (0)

#define TEST_END(name) do { \
    if (s_failures) { \
        fprintf(stderr, "%s: %d failure(s)\n", name, s_failures); \
        return 1; \
    } \
    printf("%s: ok\n", name); \
    return 0; \
} while (0)

#endif
