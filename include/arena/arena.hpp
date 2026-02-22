#pragma once

#include <cstddef>
#include <cstdint>
#include <new>
#include <type_traits>
#include <utility>
#include <vector>

namespace arena
{
  /**
   * @brief A fast bump-pointer arena allocator.
   *
   * This arena allocates memory linearly from a fixed-size buffer.
   * Individual frees are not supported; instead you can:
   * - reset() the whole arena at once
   * - use Mark + rewind() for checkpoints
   * - use Scope for RAII-based temporary allocations (mark/rewind)
   *
   * @note This allocator is not thread-safe.
   * @note This arena does not call destructors automatically for objects
   *       created with make()/make_array(). Use it for temporary lifetimes
   *       or types that don't require destruction, or manage destruction yourself.
   */
  class Arena final
  {
  public:
    /// @brief Byte storage type used by the internal buffer.
    using byte = std::byte;

    /**
     * @brief Construct an arena with a fixed byte capacity.
     * @param capacity_bytes Total bytes available for allocations.
     */
    explicit Arena(std::size_t capacity_bytes)
        : buffer_(capacity_bytes), offset_(0)
    {
    }

    /// @brief Construct an empty arena (capacity = 0).
    Arena() : Arena(0) {}

    Arena(const Arena &) = delete;
    Arena &operator=(const Arena &) = delete;

    /**
     * @brief Move-construct an arena.
     * @note Any pointers obtained from the moved-from arena become invalid.
     */
    Arena(Arena &&other) noexcept
        : buffer_(std::move(other.buffer_)), offset_(other.offset_)
    {
      other.offset_ = 0;
    }

    /**
     * @brief Move-assign an arena.
     * @note Any pointers obtained from the previous buffer become invalid.
     */
    Arena &operator=(Arena &&other) noexcept
    {
      if (this != &other)
      {
        buffer_ = std::move(other.buffer_);
        offset_ = other.offset_;
        other.offset_ = 0;
      }
      return *this;
    }

    /**
     * @brief Reset the arena to empty (all allocations become invalid).
     *
     * This is O(1).
     */
    void reset() noexcept { offset_ = 0; }

    /// @return Total capacity in bytes.
    [[nodiscard]] std::size_t capacity() const noexcept { return buffer_.size(); }

    /// @return Number of bytes currently used.
    [[nodiscard]] std::size_t used() const noexcept { return offset_; }

    /// @return Remaining capacity in bytes.
    [[nodiscard]] std::size_t remaining() const noexcept { return capacity() - used(); }

    /// @return True if no bytes are currently allocated.
    [[nodiscard]] bool empty() const noexcept { return offset_ == 0; }

    /**
     * @brief Allocate a raw memory block with alignment.
     * @param size Requested size in bytes (0 will be treated as 1).
     * @param alignment Requested alignment (must be a power of two).
     * @return Pointer to the allocated block.
     * @throws std::bad_alloc If there is not enough space or alignment is invalid.
     */
    [[nodiscard]] void *allocate(std::size_t size, std::size_t alignment = alignof(std::max_align_t))
    {
      void *p = try_allocate(size, alignment);
      if (!p)
        throw std::bad_alloc{};
      return p;
    }

    /**
     * @brief Try to allocate a raw memory block with alignment.
     * @param size Requested size in bytes (0 will be treated as 1).
     * @param alignment Requested alignment (must be a power of two).
     * @return Pointer to the allocated block, or nullptr on failure.
     *
     * Failure happens if:
     * - alignment is not a power of two
     * - the arena does not have enough remaining space
     */
    [[nodiscard]] void *try_allocate(std::size_t size, std::size_t alignment = alignof(std::max_align_t)) noexcept
    {
      if (size == 0)
        size = 1;

      if (!is_power_of_two(alignment))
        return nullptr;

      const std::size_t base = reinterpret_cast<std::size_t>(buffer_.data());
      const std::size_t current = base + offset_;
      const std::size_t aligned = align_up(current, alignment);
      const std::size_t new_offset = (aligned - base) + size;

      if (new_offset > buffer_.size())
        return nullptr;

      offset_ = new_offset;
      return reinterpret_cast<void *>(aligned);
    }

    /**
     * @brief Construct an object of type T inside the arena.
     * @tparam T The object type to construct.
     * @tparam Args Constructor argument types.
     * @param args Constructor arguments forwarded to T's constructor.
     * @return Pointer to the constructed object.
     *
     * @warning The object's destructor is NOT called automatically by Arena.
     *          Use this only when the object lifetime is bounded by reset()/rewind(),
     *          or when T is trivially destructible, or when you manually destroy it.
     */
    template <class T, class... Args>
    [[nodiscard]] T *make(Args &&...args)
    {
      static_assert(!std::is_void_v<T>, "T must not be void");
      void *mem = allocate(sizeof(T), alignof(T));
      return ::new (mem) T(std::forward<Args>(args)...);
    }

    /**
     * @brief Allocate and default-construct an array of T in the arena.
     * @tparam T The element type.
     * @param count Number of elements.
     * @return Pointer to the first element, or nullptr if count == 0.
     *
     * If T is trivially default constructible, elements are left uninitialized
     * (fast path). Otherwise, each element is default-constructed.
     *
     * @warning Destructors are NOT called automatically by Arena.
     */
    template <class T>
    [[nodiscard]] T *make_array(std::size_t count)
    {
      static_assert(!std::is_void_v<T>, "T must not be void");
      if (count == 0)
        return nullptr;

      void *mem = allocate(sizeof(T) * count, alignof(T));
      T *ptr = static_cast<T *>(mem);

      if constexpr (std::is_trivially_default_constructible_v<T>)
      {
        return ptr;
      }
      else
      {
        for (std::size_t i = 0; i < count; ++i)
          ::new (static_cast<void *>(ptr + i)) T();
        return ptr;
      }
    }

    /**
     * @brief Check whether a pointer lies within the arena buffer.
     * @param p Pointer to test.
     * @return True if p is inside [buffer_begin, buffer_end).
     */
    [[nodiscard]] bool owns(const void *p) const noexcept
    {
      const auto *b = reinterpret_cast<const std::uint8_t *>(buffer_.data());
      const auto *e = b + buffer_.size();
      const auto *x = reinterpret_cast<const std::uint8_t *>(p);
      return x >= b && x < e;
    }

    /**
     * @brief A checkpoint representing the current allocation offset.
     *
     * Use mark() to capture a checkpoint, and rewind() to restore it.
     */
    struct Mark
    {
      /// @brief Saved offset in bytes.
      std::size_t offset = 0;
    };

    /**
     * @brief Capture the current arena offset.
     * @return A Mark that can be used with rewind().
     */
    [[nodiscard]] Mark mark() const noexcept { return Mark{offset_}; }

    /**
     * @brief Rewind the arena back to a previously captured mark.
     * @param m The mark to restore.
     *
     * If m.offset is out of range, this function does nothing.
     *
     * @note All allocations performed after the mark become invalid.
     */
    void rewind(Mark m) noexcept
    {
      if (m.offset <= buffer_.size())
        offset_ = m.offset;
    }

    /**
     * @brief RAII helper that rewinds the arena when it goes out of scope.
     *
     * Typical usage:
     * @code
     * arena::Arena a(4096);
     * {
     *   arena::Arena::Scope scope(a);
     *   auto* tmp = a.make<Foo>(...);
     * } // automatic rewind here
     * @endcode
     */
    class Scope final
    {
    public:
      /**
       * @brief Create a scope that will rewind the arena on destruction.
       * @param a Arena to scope.
       */
      explicit Scope(Arena &a) noexcept : a_(&a), m_(a.mark()) {}

      Scope(const Scope &) = delete;
      Scope &operator=(const Scope &) = delete;

      /**
       * @brief Move-construct a scope.
       *
       * Ownership of the rewind responsibility is transferred.
       */
      Scope(Scope &&other) noexcept : a_(other.a_), m_(other.m_)
      {
        other.a_ = nullptr;
      }

      /**
       * @brief Move-assign a scope.
       *
       * If this scope is active, it rewinds its arena before taking ownership
       * of the other scope's arena.
       */
      Scope &operator=(Scope &&other) noexcept
      {
        if (this != &other)
        {
          if (a_)
            a_->rewind(m_);
          a_ = other.a_;
          m_ = other.m_;
          other.a_ = nullptr;
        }
        return *this;
      }

      /// @brief Rewind the arena back to the saved mark.
      ~Scope()
      {
        if (a_)
          a_->rewind(m_);
      }

    private:
      Arena *a_;
      Mark m_;
    };

  private:
    /**
     * @brief Return true if x is a power of two.
     */
    static constexpr bool is_power_of_two(std::size_t x) noexcept
    {
      return x != 0 && (x & (x - 1)) == 0;
    }

    /**
     * @brief Align value up to the next multiple of alignment.
     * @param value Value to align.
     * @param alignment Power-of-two alignment.
     * @return Aligned value.
     */
    static constexpr std::size_t align_up(std::size_t value, std::size_t alignment) noexcept
    {
      return (value + (alignment - 1)) & ~(alignment - 1);
    }

    std::vector<byte> buffer_;
    std::size_t offset_;
  };
} // namespace arena
