#pragma once

#include <new>
#include <thread>
#include <malloc.h> // memalign, _aligned_malloc


// ref: https://en.cppreference.com/w/cpp/header/memory_resource
namespace zstl {

namespace pmr {

// memory_resource
// ref: https://en.cppreference.com/w/cpp/memory/memory_resource
class memory_resource {
public:
    memory_resource() = default;

    memory_resource(const memory_resource& ) = default;

    virtual ~memory_resource();

    // to do: Exceptions
    // Throws an exception if storage of the requested size and alignment cannot be obtained
    void* allocate(
        std::size_t bytes,
        std::size_t alignment = alignof(std::max_align_t)
    ) {
        if (bytes == 0uz) { return nullptr; }

        return do_allocate(bytes, alignment);
    }

    void deallocate(
        void *p,
        std::size_t bytes,
        std::size_t alignment = alignof(std::max_align_t)
    ) {
        if (!p) { return ; }

        return do_deallocate(p, bytes, alignment);
    }

    bool is_equal(const memory_resource &other) const noexcept {
        return do_is_equal(other);
    }

private:
    virtual void *do_allocate(std::size_t bytes, std::size_t alignment) = 0;

    virtual void do_deallocate(void *p, std::size_t bytes, std::size_t alignment) = 0;

    virtual bool do_is_equal(const memory_resource &other) const noexcept = 0;
};

inline bool operator==(
    const memory_resource &a,
    const memory_resource &b
) noexcept {
    return a.is_equal(b);
}




// global memory resources

// NewDeleteResource
class NewDeleteResource : public memory_resource {
    void *do_allocate(std::size_t size, std::size_t alignment) override {
#if defined(PBRT_HAVE__ALIGNED_MALLOC)
        return _aligned_malloc(size, alignment);
#elif defined(PBRT_HAVE_POSIX_MEMALIGN)
        void *ptr;
        if (alignment < sizeof(void *))
            return malloc(size);
        if (posix_memalign(&ptr, alignment, size) != 0)
            ptr = nullptr;
        return ptr;
#else
        return memalign(alignment, size);
#endif
    }

    void do_deallocate(void *ptr, std::size_t bytes, std::size_t alignment) override {
        if (!ptr) { return ; }
#if defined(PBRT_HAVE__ALIGNED_MALLOC)
        _aligned_free(ptr);
#else
        free(ptr);
#endif
    }

    bool do_is_equal(const memory_resource &other) const noexcept override {
        return this == &other;
    }
};
static NewDeleteResource *ndr;


// ref: https://en.cppreference.com/w/cpp/memory/new_delete_resource
inline memory_resource *new_delete_resource() noexcept {
    if (!ndr) {
        ndr = new NewDeleteResource;
    }

    return ndr;
}

// TODO
// ref: https://en.cppreference.com/w/cpp/memory/null_memory_resource
memory_resource* null_memory_resource() noexcept;

// TODO
// ref: https://en.cppreference.com/w/cpp/memory/set_default_resource
memory_resource* set_default_resource(memory_resource *r) noexcept;

// TODO
// ref: https://en.cppreference.com/w/cpp/memory/get_default_resource
memory_resource* get_default_resource() noexcept;




// pool resource classes

// ref: https://en.cppreference.com/w/cpp/memory/pool_options
struct pool_options {
    std::size_t max_blocks_per_chunk { 0uz };
    std::size_t largest_required_pool_block { 0uz };
};

// TODO
// ref: https://en.cppreference.com/w/cpp/memory/synchronized_pool_resource
class synchronized_pool_resource;

// TODO
// ref: https://en.cppreference.com/w/cpp/memory/unsynchronized_pool_resource
class unsynchronized_pool_resource;


// ref: https://en.cppreference.com/w/cpp/memory/monotonic_buffer_resource
class alignas(64) monotonic_buffer_resource : public memory_resource {
public:
    monotonic_buffer_resource()
        : monotonic_buffer_resource(get_default_resource())
    {}

    explicit monotonic_buffer_resource(memory_resource *upstream)
        : upstream(upstream)
    {
#ifndef NDEBUG
        this->constructTID = std::this_thread::get_id();
#endif
    }

    explicit monotonic_buffer_resource(std::size_t initial_size)
        : monotonic_buffer_resource(
            initial_size,
            get_default_resource()
        )
    {}

    monotonic_buffer_resource(
        std::size_t initial_size,
        memory_resource *upstream
    )
        : block_size(initial_size)
        , upstream(upstream)
    {
#ifndef NDEBUG
        this->constructTID = std::this_thread::get_id();
#endif
    }

    // TODO
    monotonic_buffer_resource(void *buffer, std::size_t buffer_size)
        : monotonic_buffer_resource(
            buffer,
            buffer_size,
            get_default_resource()
        )
    {}

    // TODO
    monotonic_buffer_resource(
        void *buffer,
        std::size_t buffer_size,
        memory_resource *upstream
    );

    monotonic_buffer_resource(const monotonic_buffer_resource& ) = delete;

    virtual ~monotonic_buffer_resource() {
        this->release();
    }

    monotonic_buffer_resource operator=(const monotonic_buffer_resource &) = delete;

    void release() {
        block *b = this->block_list;
        while (b) {
            block *next = b->next;
            free_block(b);
            b = next;
        }
        this->block_list = nullptr;
        this->current = nullptr;
    }

    memory_resource *upstream_resource() const {
        return this->upstream;
    }

protected:
    void *do_allocate(std::size_t bytes, std::size_t align) override;

    void do_deallocate(void *p, std::size_t bytes, std::size_t alignment) override {
        if (bytes > this->block_size) {
            // do_allocate() passes large allocations on to the upstream memory resource,
            // so we might as well deallocate when it's possible.
            this->upstream->deallocate(p, bytes);
        }
    }

    bool do_is_equal(const memory_resource &other) const noexcept override {
        return this == &other;
    }

private:
    struct block {
        void *ptr { nullptr };
        std::size_t size { 0uz };
        block *next { nullptr };
    };

    block *allocate_block(std::size_t size) {
        // Single allocation for both the block and its memory. This means
        // that strictly speaking MemoryBlock::ptr is redundant, but let's not get too
        // fancy here...
        block *b = static_cast<block *>(
            this->upstream->allocate(sizeof(block) + size, alignof(block))
        );

        b->ptr = reinterpret_cast<char *>(b) + sizeof(block);
        b->size = size;
        b->next = this->block_list;
        this->block_list = b;

        return b;
    }

    void free_block(block *b) {
        this->upstream->deallocate(b, sizeof(block) + b->size);
    }

#ifndef NDEBUG
    std::thread::id constructTID;
#endif
    memory_resource *upstream { nullptr };
    std::size_t block_size { 256 * 1024uz };
    block *current { nullptr };
    std::size_t current_pos { 0uz };
    block *block_list { nullptr };
};




// ref: https://en.cppreference.com/w/cpp/memory/polymorphic_allocator
template <class Tp = std::byte>
class polymorphic_allocator {
private:
    memory_resource *memoryResource;

public:
    using value_type = Tp;

    polymorphic_allocator() noexcept {
        memoryResource = new_delete_resource();
    }

    polymorphic_allocator(memory_resource *r)
        : memoryResource(r)
    {}

    polymorphic_allocator(const polymorphic_allocator &other) = default;

    template <class U>
    polymorphic_allocator(const polymorphic_allocator<U> &other) noexcept
        : memoryResource(other.resource())
    {}

    polymorphic_allocator &operator=(const polymorphic_allocator& ) = delete;


    // member functions
    [[nodiscard]] Tp *allocate(std::size_t n) {
        return static_cast<Tp *>(
            this->resource()->allocate(n * sizeof(Tp), alignof(Tp))
        );
    }

    void deallocate(Tp *p, std::size_t n) {
        this->resource()->deallocate(p, n * sizeof(Tp), alignof(Tp));
    }

    // TODO
    // ref: https://en.cppreference.com/w/cpp/memory/polymorphic_allocator/construct
    template <class U, class... Args>
    void construct(U *p, Args &&...args) {
        ::new ((void *)p) U(std::forward<Args>(args)...);
    }

    template <class U>
    void destroy(U *p) {
        p->~U();
    }

    void *allocate_bytes(
        std::size_t nbytes,
        std::size_t alignment = alignof(std::max_align_t)
    ) {
        return this->resource()->allocate(nbytes, alignment);
    }

    void deallocate_bytes(
        void *p,
        std::size_t nbytes,
        std::size_t alignment = alignof(std::max_align_t)
    ) {
        return this->resource()->deallocate(p, nbytes, alignment);
    }

    template <class U>
    U *allocate_object(std::size_t n = 1uz) {
        if (std::numeric_limits<std::size_t>::max() / sizeof(U) < n) {
            throw std::bad_array_new_length();
        }

        return static_cast<U *>(allocate_bytes(n * sizeof(U), alignof(U)));
    }

    template <class U>
    void deallocate_object(U *p, std::size_t n = 1uz) {
        deallocate_bytes(p, n * sizeof(U), alignof(U));
    }

    template <class U, class... Args>
    U *new_object(Args &&...args) {
        // NOTE: this doesn't handle constructors that throw exceptions...
        U *p = allocate_object<U>();
        this->construct(p, std::forward<Args>(args)...);

        return p;
    }

    template <class U>
    void delete_object(U *p) {
        destroy(p);
        deallocate_object(p);
    }

    // polymorphic_allocator select_on_container_copy_construction() const;

    memory_resource *resource() const {
        return memoryResource;
    }
};

template <class T1, class T2>
bool operator==(
    const polymorphic_allocator<T1> &a,
    const polymorphic_allocator<T2> &b
) noexcept {
    return a.resource() == b.resource();
}

} // namespace pmr end

} // namespace zstl end
