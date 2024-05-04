#include "gc-arenas.h"
#include "gc.h"
#ifndef _OS_WINDOWS_
#  include <sys/resource.h>
#endif
#include <vector>
using std::vector;

/*
An Arena is a contiguous block of memory (a buffer), preallocated in page-size chunks.
If we fill up a page, we allocate another. This is very similar to the existing GC Pools,
except that the Arena can be used for *time-based* allocations: You can allocate onto the
arena, and when you're done with it, free it in one go.

During a GC collection, the Arena acts as a root, potentially keeping objects alive until
the entire arena is freed.

TODO: We use one Arena per thread, to reduce contention and increase locality.
TODO: Grow the arena

*/
static bool gc_short_arena_enabled = false;


jl_value_t* gc_short_arena_alloc_inner(jl_ptls_t ptls, int osize);
void gc_free_short_arena_inner();

void initialize_arena();

#define ARENA_SIZE (GC_PAGE_SZ * 32)
// Preallocate a huge arena
// #define ARENA_SIZE (2 << 30)

// Interface
extern "C" {

JL_DLLEXPORT void jl_gc_short_arena_set_enabled(int8_t enabled) {
    gc_short_arena_enabled = enabled;
    if (enabled == false) {
        return;
    }
    initialize_arena();
}
JL_DLLEXPORT int8_t jl_gc_short_arena_is_enabled(void) {
    return gc_short_arena_enabled;
}
JL_DLLEXPORT void jl_gc_short_arena_free(void) {
    return gc_free_short_arena_inner();
}
jl_value_t* gc_short_arena_alloc(jl_ptls_t ptls, int osize) {
    return gc_short_arena_alloc_inner(ptls, osize);
}

}  // extern "C"

// Implementation


// TODO: Integrate with GC Pages?
static vector<char*> arena_pages;
static char* arena_next_alloc = 0;

void add_page_to_arena() {
    char *arena = (char*)mmap(0, ARENA_SIZE, PROT_READ | PROT_WRITE,
                            MAP_NORESERVE | MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (arena == MAP_FAILED) {
        // TODO
        gc_short_arena_enabled = false;
        abort();
        return;
    }
    arena_next_alloc = arena;
    arena_pages.push_back(arena);
}

void initialize_arena() {
    if (arena_pages.size() == 0) {
        add_page_to_arena();
    }
}

bool arena_no_room(int osize) {
    return arena_next_alloc + osize >= arena_pages.back() + ARENA_SIZE;
}

jl_value_t* gc_short_arena_alloc_inner(jl_ptls_t ptls, int osize) {
    if (arena_no_room(osize)) {
        jl_safe_printf("Adding page to arena.\n");
        add_page_to_arena();
        if (arena_no_room(osize)) {
            jl_safe_printf("Fatal: Couldn't fit %d in arena.\n", osize);
            jl_throw_out_of_memory_error();
        }
    }
    // jl_safe_printf("ALLOC! size: %d\n", osize);
    jl_taggedvalue_t *v = (jl_taggedvalue_t*)arena_next_alloc;
    // Need to leave size for the julia object header
    arena_next_alloc += osize;
    return jl_valueof(v);
}
void gc_free_short_arena_inner(void) {
    for (char* arena : arena_pages) {
        // TODO: Share all the fancy MADV_FREE stuff w/ GC pages.
        madvise(arena, ARENA_SIZE, MADV_DONTNEED);
    }
    arena_pages.clear();
    arena_next_alloc = 0;
}




