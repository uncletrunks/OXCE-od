#pragma once

#if __has_cpp_attribute(fallthrough)
#define FALLTHROUGH [[fallthrough]]
#elif __has_cpp_attribute(gnu::fallthrough)
#define FALLTHROUGH [[gnu::fallthrough]]
#elif __has_cpp_attribute(clang::fallthrough)
#define FALLTHROUGH [[clang::fallthrough]]
#else
#define FALLTHROUGH
#endif
