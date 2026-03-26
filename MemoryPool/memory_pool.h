#pragma once
#ifndef _COMPONENTS_MEMORY_POOL_H_
  #define _COMPONENTS_MEMORY_POOL_H_

  #include <cstddef>
  #include <cstdint>
  #include <type_traits>
  #include <utility>

template <size_t N>
struct AlignedChecker {
    static constexpr bool value = N > 0 && ((N & (N - 1)) == 0);
};

template <typename T, int BlockSize = 4'096>
class MemoryPool {
public:
    using value_type = T;
    using size_type = size_t;
    using difference_type = ptrdiff_t;
    using pointer = T*;
    using reference = T&;
    using const_pointer = const T*;
    using const_reference = const T&;
    using propagate_on_container_swap = std::true_type;
    using propagate_on_container_copy_assignment = std::false_type;
    using propagate_on_container_move_assignment = std::true_type;

    template <typename U>
    struct rebind {
        using other = MemoryPool<U>;
    };

public:
    MemoryPool() noexcept;
    ~MemoryPool() noexcept;
    MemoryPool(const MemoryPool& __memoryPool) noexcept;
    MemoryPool(MemoryPool&& __memoryPool) noexcept;

    template <typename U>
    MemoryPool(const MemoryPool<U>& __memoryPool) noexcept;

    MemoryPool& operator=(MemoryPool&& __memoryPool) noexcept;
    MemoryPool& operator=(const MemoryPool& __memoryPool) = delete;

    pointer address(reference __obj) const noexcept;
    const_pointer address(const_reference __obj) const noexcept;

    pointer allocate(size_type __n = 1, const_pointer __hint = nullptr);
    void deallocate(pointer __p, size_type __n = 1);

    size_type max_size() const noexcept;

    template <typename U, typename... Args>
    void construct(U* __p, Args&&... __args);

    template <typename U>
    void destroy(U* __p);

    template <typename... Args>
    pointer new_element(Args&&... __args);

    void delete_element(pointer __p);

private:
    union Slot {
        value_type element;
        Slot* next;
    };

    using data_pointer_ = char*;
    using slot_pointer_ = Slot*;
    using slot_type_ = Slot;

    slot_pointer_ current_block_;
    slot_pointer_ current_slot_;
    slot_pointer_ last_slot_;
    slot_pointer_ free_slot_;

    void allocate_block();
    size_type padding_size(data_pointer_ __p, size_type __align) const noexcept;

    static_assert(BlockSize >= 2 * sizeof(slot_type_),
                  "BlockSize is too small.");
};

template <typename T, int BlockSize>
inline void MemoryPool<T, BlockSize>::allocate_block() {
    data_pointer_ new_block =
        reinterpret_cast<data_pointer_>(::operator new(BlockSize));

    reinterpret_cast<slot_pointer_>(new_block)->next = current_block_;
    current_block_ = reinterpret_cast<slot_pointer_>(new_block);
    data_pointer_ tail = new_block + sizeof(slot_pointer_);

    static constexpr size_type ALIGNED_SIZE = alignof(slot_type_);

    if constexpr (AlignedChecker<ALIGNED_SIZE>::value) {
        uintptr_t res = reinterpret_cast<uintptr_t>(tail);
        current_slot_ = reinterpret_cast<slot_pointer_>(
            (res + ALIGNED_SIZE - 1) & ~(ALIGNED_SIZE - 1));
    } else {
        size_type tail_padding = padding_size(tail, ALIGNED_SIZE);
        current_slot_ = reinterpret_cast<slot_pointer_>(tail + tail_padding);
    }

    last_slot_ = reinterpret_cast<slot_pointer_>(new_block + BlockSize -
                                                 sizeof(slot_type_) + 1);
}

template <typename T, int BlockSize>
inline typename MemoryPool<T, BlockSize>::size_type
MemoryPool<T, BlockSize>::padding_size(data_pointer_ __p,
                                       size_type __align) const noexcept {
    uintptr_t res = reinterpret_cast<uintptr_t>(__p);
    return (__align - res) % __align;
}

template <typename T, int BlockSize>
inline MemoryPool<T, BlockSize>::MemoryPool() noexcept {
    current_block_ = nullptr;
    current_slot_ = nullptr;
    last_slot_ = nullptr;
    free_slot_ = nullptr;
}

template <typename T, int BlockSize>
inline MemoryPool<T, BlockSize>::~MemoryPool<T, BlockSize>() noexcept {
    slot_pointer_ cur = current_block_;
    while (cur != nullptr) {
        slot_pointer_ next = cur->next;
        ::operator delete(cur);
        cur = next;
    }
}

template <typename T, int BlockSize>
inline MemoryPool<T, BlockSize>::MemoryPool(
    const MemoryPool& __memoryPool) noexcept
    : MemoryPool() {}

template <typename T, int BlockSize>
inline MemoryPool<T, BlockSize>::MemoryPool(
    MemoryPool&& __memoryPool) noexcept {
    current_block_ = __memoryPool.current_block_;
    current_slot_ = __memoryPool.current_slot_;
    last_slot_ = __memoryPool.last_slot_;
    free_slot_ = __memoryPool.free_slot_;
    __memoryPool.current_block_ = nullptr;
    __memoryPool.current_slot_ = nullptr;
    __memoryPool.last_slot_ = nullptr;
    __memoryPool.free_slot_ = nullptr;
}

template <typename T, int BlockSize>
template <typename U>
inline MemoryPool<T, BlockSize>::MemoryPool(
    const MemoryPool<U>& __memoryPool) noexcept
    : MemoryPool() {}

template <typename T, int BlockSize>
inline MemoryPool<T, BlockSize>& MemoryPool<T, BlockSize>::operator=(
    MemoryPool&& __memoryPool) noexcept {
    if (this == &__memoryPool) {
        return *this;
    }
    std::swap(current_block_, __memoryPool.current_block_);
    current_slot_ = __memoryPool.current_slot_;
    last_slot_ = __memoryPool.last_slot_;
    free_slot_ = __memoryPool.free_slot_;
    return *this;
}

template <typename T, int BlockSize>
inline typename MemoryPool<T, BlockSize>::pointer
MemoryPool<T, BlockSize>::address(reference __obj) const noexcept {
    return &__obj;
}

template <typename T, int BlockSize>
inline typename MemoryPool<T, BlockSize>::const_pointer
MemoryPool<T, BlockSize>::address(const_reference __obj) const noexcept {
    return &__obj;
}

template <typename T, int BlockSize>
inline typename MemoryPool<T, BlockSize>::pointer
MemoryPool<T, BlockSize>::allocate(size_type __n, const_pointer __hint) {
    if (free_slot_ != nullptr) {
        pointer res = reinterpret_cast<pointer>(free_slot_);
        free_slot_ = free_slot_->next;
        return res;
    } else {
        if (current_slot_ >= last_slot_) allocate_block();
        return reinterpret_cast<pointer>(current_slot_++);
    }
}

template <typename T, int BlockSize>
inline void MemoryPool<T, BlockSize>::deallocate(pointer __p, size_type __n) {
    if (__p != nullptr) {
        reinterpret_cast<slot_pointer_>(__p)->next = free_slot_;
        free_slot_ = reinterpret_cast<slot_pointer_>(__p);
    }
}

template <typename T, int BlockSize>
inline typename MemoryPool<T, BlockSize>::size_type
MemoryPool<T, BlockSize>::max_size() const noexcept {
    size_type max_blocks = -1 / BlockSize;
    return (BlockSize - sizeof(data_pointer_)) / sizeof(slot_type_) *
           max_blocks;
}

template <typename T, int BlockSize>
template <typename U, typename... Args>
inline void MemoryPool<T, BlockSize>::construct(U* __p, Args&&... __args) {
    new (__p) U(std::forward<Args>(__args)...);
}

template <typename T, int BlockSize>
template <typename U>
inline void MemoryPool<T, BlockSize>::destroy(U* __p) {
    __p->~U();
}

template <typename T, int BlockSize>
template <typename... Args>
inline typename MemoryPool<T, BlockSize>::pointer
MemoryPool<T, BlockSize>::new_element(Args&&... __args) {
    pointer res = allocate();
    construct(res, std::forward<Args>(__args)...);
    return res;
}

template <typename T, int BlockSize>
inline void MemoryPool<T, BlockSize>::delete_element(pointer __p) {
    if (__p != nullptr) {
        destroy(__p);
        deallocate(__p);
    }
}
#endif
