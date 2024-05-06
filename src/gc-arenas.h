// This file is a part of Julia. License is MIT: https://julialang.org/license

// TODO:
//  - Need to disable the arena whenever entering runtime code, like the compiler. Otherwise
//    you can never turn it off, since the compiler objects are allocated in there!


#ifndef JL_GC_ARENAS_H
#define JL_GC_ARENAS_H

#include "julia.h"

#ifdef __cplusplus
extern "C" {
#endif

JL_DLLEXPORT void jl_gc_short_arena_set_enabled(int8_t);
JL_DLLEXPORT int8_t jl_gc_short_arena_is_enabled(void);
JL_DLLEXPORT void jl_gc_short_arena_free(void);

jl_value_t* gc_short_arena_alloc(jl_ptls_t ptls, int osize);

void gc_free_short_arena(void);

#ifdef __cplusplus
}
#endif

#endif  // JL_GC_ARENAS_H
