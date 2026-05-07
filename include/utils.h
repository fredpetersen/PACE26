#pragma once

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