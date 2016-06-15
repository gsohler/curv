// Copyright Doug Moen 2016.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENCE.md or http://www.boost.org/LICENSE_1_0.txt

#ifndef AUX_SHARED_H
#define AUX_SHARED_H

#include <boost/intrusive_ptr.hpp>
#include <cstdint>
#include <new>

namespace aux {

/// Cheap alternative to `std::shared_ptr`.
///
/// Memory overhead is one pointer, instead of two for `std::shared_ptr`.
/// For further economies, see `aux::Shared_Base`.
template<typename T> using Shared_Ptr = boost::intrusive_ptr<T>;

template<typename T, class... Args> Shared_Ptr<T> make_shared(Args&&... args)
{
    void* raw = std::malloc(sizeof(T));
    if (raw == nullptr)
        throw std::bad_alloc();
    T* ptr = new(raw) T(args...);
    return Shared_Ptr<T>(ptr);
}

/// Common base class for cheap reference-counted objects.
///
/// For performance reasons, the use_count is incremented and decremented
/// non-atomically, which is not thread safe. That's what you typically
/// need in cases where `std::shared_ptr` is too expensive.
///
/// The memory overhead is one use_count, instead of two for `std::shared_ptr`.
/// Plus I'm forcing the use of a vtable. I specifically want the vtable pointer
/// (64 bits) to precede the 32 bit use_count, for alignment reasons. This
/// leaves room for a subclass to add another 32 bits of data without creating
/// an alignment hole in the memory layout on 64 bit systems. I use this to
/// reduce memory consumption for curv::Ref_Value, which puts a 32 bit type code
/// in this location.
///
/// Suppose `Foo` is a subclass of `Shared_Base`.
/// 1. To pass an instance of `Foo` to a function `f`, pass it by reference
///    (`Foo&`), not by value (it's non-copyable), and not by `Shared_Ptr<Foo>`
///    in most cases (too expensive: that would increment/decrement the
///    reference count).
/// 2. As a special case, for constructors and factory functions, where the
///    normal usage is for the caller to pass ownership of a Shared_Ptr to
///    the constructor, then declare the argument as `Shared_Ptr<Foo>`.
/// 3. In case #1, if `f` needs to keep a reference alive after the function
///    has returned, then it can create a Shared_Ptr to the instance.
///    (That's not possible with `std::shared_ptr`.) The cost of constructing
///    a Shared_Ptr is only incurred when required, and the interface to `f`
///    doesn't need to expose this implementation detail, by being changed
///    to accept a `Shared_Ptr<Foo>` argument. This is particular relevant
///    when a member function needs to construct Shared_Ptr<Foo>(this),
///    something impossible with std::shared_ptr.
/// 4. As a corrolary to #3, it is DANGEROUS to construct a static or auto
///    instance of `Foo`, because the technique of #3 could result in an attempt
///    to delete a static or auto variable, leading to memory corruption.
///    Therefore, all instances of `Foo` should be constructed using
///    `aux::make_shared`.
/// 5. I wish I could enforce the requirement to not construct auto or static
///    instances. The class is non-copyable, but that's not enough.
struct Shared_Base
{
    Shared_Base() : use_count(0) {}
    virtual ~Shared_Base() {}
    mutable std::uint32_t use_count;
private:
    // Shared_Base is non-copyable.
    Shared_Base(const Shared_Base&) = delete;
    Shared_Base& operator=(const Shared_Base&) = delete;
};

inline void intrusive_ptr_add_ref(const Shared_Base* p)
{
    ++p->use_count;
}

inline void intrusive_ptr_release(const Shared_Base* p)
{
    if (--p->use_count == 0) {
        p->~Shared_Base();
        std::free((void*)p);
    }
}

// How to support atomic refcount update in the future.
// * Add a Shared_Atomic_Base class, which always uses atomic updates.
// * Add a Shared_Maybe_Atomic_Base class, which reserves a bit in the
//   use_count to indicate whether the use_count should be updated atomically.
//   When an object is first constructed, it's only accessible from one thread.
//   Later, it might "escape" and become visible to more than thread.
//   If and when this happens, we mark the object as atomic.

} // namespace aux
#endif // header guard