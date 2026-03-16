#include <cstdlib>
#include <gc.h>
#include <new>

// =============================================================================
// 全局 new / delete 接管 (专门面向 Boehm GC)
// =============================================================================

#ifdef PYLANG_USE_Boehm_GC

void *operator new(std::size_t size)
{
    void *ptr = GC_MALLOC(size);
    if (!ptr) throw std::bad_alloc();
    return ptr;
}

void *operator new[](std::size_t size)
{
    void *ptr = GC_MALLOC(size);
    if (!ptr) throw std::bad_alloc();
    return ptr;
}

void operator delete(void *ptr) noexcept
{
    GC_FREE(ptr);
}

void operator delete[](void *ptr) noexcept { GC_FREE(ptr); }

void operator delete(void *ptr, std::size_t) noexcept { GC_FREE(ptr); }

void operator delete[](void *ptr, std::size_t) noexcept { GC_FREE(ptr); }

#endif // PYLANG_USE_Boehm_GC