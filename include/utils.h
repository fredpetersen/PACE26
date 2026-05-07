#pragma once

#include <cstdint>
#include <iostream>
#include <string>

// Compile-time toggle. Define DEBUG_LOG to a non-zero value to enable.
#ifndef DEBUG_LOG
#define DEBUG_LOG 0
#endif

#if DEBUG_LOG
void debug_impl(const std::string& msg);
#define debug(msg) ::debug_impl((msg))
#else
#define debug(msg) ((void)0)
#endif

// Symmetric 64-bit cherry hash over two label strings.
// Returns non-zero (0 is reserved as the "unset" sentinel for TreeNode::cpsHash).
inline uint64_t computeCpsHash(const std::string& a, const std::string& b) {
    uint64_t h1 = std::hash<std::string>{}(a);
    uint64_t h2 = std::hash<std::string>{}(b);
    if (h1 > h2) std::swap(h1, h2);
    uint64_t h = h1 ^ (h2 + 0x9e3779b97f4a7c15ULL + (h1 << 6) + (h1 >> 2));
    return h == 0 ? 1 : h;
}