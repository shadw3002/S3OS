#pragma once

#include "type.h"

constexpr u8 MAG_CH_PANIC  = '\002';
constexpr u8 MAG_CH_ASSERT = '\003';

// __FILE__ : 被展开的文件
// __BASE_FILE__: .h
// __LINE__: .cpp
#define ASSERT
#ifdef ASSERT
void assertion_failure(char *exp, char *file, char *base_file, int line);
#define assert(exp)  if (exp) ; \
        else assertion_failure(#exp, __FILE__, __BASE_FILE__, __LINE__)
#else
#define assert(exp)
#endif

void panic(const char *fmt, ...);