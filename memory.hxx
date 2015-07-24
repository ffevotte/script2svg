#pragma once

// Possibly Owning ptr
template <typename T>
class POptr {
public:
  POptr ()
    : ptr_ (0),
      owner_ (false)
  {}

  POptr (T* ptr, bool owner)
    : ptr_ (ptr),
      owner_ (owner)
  {}

  // Non copyable
  POptr (const POptr & other) = delete;

  ~POptr () {
    if (owner_)
      delete (ptr_);
  }

  void reset (T* ptr, bool owner) {
    if (owner_)
      delete (ptr_);
    ptr_   = ptr;
    owner_ = owner;
  }

  T* get () const { return ptr_; }

private:
  T * ptr_;
  bool owner_;
};
