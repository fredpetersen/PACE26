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

// 64-bit symmetric hash mixer used everywhere in the solver. (a,b) and (b,a)
// hash identically. Returns non-zero (0 is reserved as the "unset" sentinel
// for TreeNode::subtreeHash).
inline uint64_t mixHashes(uint64_t h1, uint64_t h2) {
    if (h1 > h2) std::swap(h1, h2);
    uint64_t h = h1 ^ (h2 + 0x9e3779b97f4a7c15ULL + (h1 << 6) + (h1 >> 2));
    return h == 0 ? 1 : h;
}

inline uint64_t hashLeafLabel(const std::string& label) {
    uint64_t h = std::hash<std::string>{}(label);
    return h == 0 ? 1 : h;
}

// Bijective 64-bit avalanche. Used to derive an independent per-leaf random
// from the leaf's label hash for set hashing (XOR of these is collision-
// resistant in practice; XOR of raw std::hash values is not).
inline uint64_t splitmix64(uint64_t x) {
    x += 0x9e3779b97f4a7c15ULL;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    x = x ^ (x >> 31);
    return x;
}

// Per-leaf 64-bit id derived from the label, suitable for XOR-based set
// hashing of leaf sets. Independent of structural hash.
inline uint64_t leafSetAtomHash(const std::string& label) {
    uint64_t h = splitmix64(hashLeafLabel(label));
    return h == 0 ? 1 : h;
}

// Backward-compatible cherry hash: equals the structural hash of an internal
// node whose two children are leaves with these labels.
inline uint64_t computeCpsHash(const std::string& a, const std::string& b) {
    return mixHashes(hashLeafLabel(a), hashLeafLabel(b));
}