// lsm_cpp/src/arena_allocator.h

#ifndef ARENA_ALLOCATOR_H
#define ARENA_ALLOCATOR_H

#include <cstddef>
#include <vector>
#include <stdexcept>

class Arena {
private:
    std::vector<char> memory;
    size_t offset = 0;

public:
    explicit Arena(size_t size_in_bytes) {
        memory.resize(size_in_bytes);
    }

    char* allocate(size_t size, size_t alignment) {
        // Align the current offset
        size_t aligned_offset = (offset + alignment - 1) & ~(alignment - 1);
        if (aligned_offset + size > memory.size()) {
            throw std::bad_alloc();
        }
        offset = aligned_offset + size;
        return memory.data() + aligned_offset;
    }

    void reset() {
        offset = 0;
    }
};


// Custom C++ allocator to be used with std::vector
template <typename T>
class ArenaAllocator {
public:
    using value_type = T;
    Arena* arena;

    ArenaAllocator(Arena& a) : arena(&a) {}

    template <typename U>
    ArenaAllocator(const ArenaAllocator<U>& other) : arena(other.arena) {}

    T* allocate(size_t n) {
        return reinterpret_cast<T*>(arena->allocate(n * sizeof(T), alignof(T)));
    }

    void deallocate(T* p, size_t n) noexcept {
        // In an arena, deallocation is a no-op. Memory is freed all at once.
    }
};

#endif // ARENA_ALLOCATOR_H