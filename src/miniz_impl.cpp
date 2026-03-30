// miniz_impl.cpp -- Compile the miniz implementation in its own translation unit.
//
// This file is ONLY compiled when cross-compiling for Windows (where system
// zlib isn't available). The engine CMakeLists conditionally adds it.
//
// DO NOT include this in native Linux builds -- they use system zlib (-lz).

#define MINIZ_IMPLEMENTATION
#include "miniz.h"
