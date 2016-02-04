//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_UTILS_ALLOC_HH
#define _TCLIB_UTILS_ALLOC_HH

#include "c/stdc.h"
#include "c/stdnew.hh"

BEGIN_C_INCLUDES
#include "utils/alloc.h"
END_C_INCLUDES

namespace tclib {

// Subclasses of this type know how to delete themselves using the default
// allocator. We need this extra machinery because, unlike plain C++, the
// allocation framework in tclib needs to know the concrete size of the type
// being deallocated so each concrete type needs special handling.
class DefaultDestructable {
public:
  // Call this instance's destructor and then deallocate this instance using the
  // default allocator. Typically, as an implementor, you want to do this by
  // calling default_delete_concrete(this).
  virtual void default_destroy() = 0;
};

// A marker that can be passed to placement new to indicate that the allocation
// should be done using the default allocator from the allocator framework.
typedef enum {
  kDefaultAlloc = 0
} default_alloc_marker_t;

// Delete a concrete instance of T, that is, an instance of T and not a subtype
// or at least one where the subtype does not add additional state beyond what
// is in T.
template <typename T>
void default_delete_concrete(T *ptr) {
  ptr->~T();
  allocator_default_free_struct(T, ptr);
}

// Delete the given object. Unlike default_delete_concrete the concrete type
// of the object does not have to be known to call this.
inline void default_delete(DefaultDestructable *that) {
  if (that != NULL)
    that->default_destroy();
}

// A reference to a value allocated with the default allocator. These are
// intended to be passed around and eventually stored in a def_ref_t which will
// deal with the lifetime of the object.
template <typename T>
class pass_def_ref_t {
public:
  explicit pass_def_ref_t(T *ptr) : ptr_(ptr) { }
  T *operator*() const { return ptr_; }

private:
  T *ptr_;
};

// A reference to a default-allocated value that will be automatically disposed
// appropriately when this ref is destroyed. The first type parameter is the
// type of the pointer contained, the second one can be used to indicate how to
// destroy the value.
template <typename T, typename D = T>
class def_ref_t {
public:
  def_ref_t() : ptr_(NULL) { }
  template <typename S>
  def_ref_t(const pass_def_ref_t<S> &pass) : ptr_(*pass) { }
  template <typename S>
  def_ref_t(const S* ptr) : ptr_(ptr) { }
  ~def_ref_t() { default_delete(static_cast<D*>(ptr_)); }
  template <typename S>
  void operator=(const pass_def_ref_t<S> &pass) { ptr_ = *pass; }
  template <typename S>
  void operator=(S *ptr) { ptr_ = ptr; }
  T *operator->() { return ptr_; }
  T *operator*() { return ptr_; }

private:
  T *ptr_;
};

} // namespace tclib

// Allocator that uses the default allocator from the C alloc framework. Note
// that this discards information about the amount allocated which we need on
// deallocation so you need to keep track of that manually somehow. But we want
// to keep track of that in any case since we want to exercise explicit control
// over how much memory gets allocated from the system.
inline void *operator new(size_t size, tclib::default_alloc_marker_t) {
  return allocator_default_malloc(size).start;
}

#endif // _TCLIB_UTILS_ALLOC_HH
