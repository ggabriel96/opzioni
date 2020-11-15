
#ifndef OPZIONI_MEMORY_H
#define OPZIONI_MEMORY_H

#include <memory>

namespace opzioni {

namespace memory {

template <typename T>
class ValuePtr {
public:
  ValuePtr() = default;
  ValuePtr(ValuePtr &&) = default;
  ValuePtr(std::nullptr_t) : ptr(nullptr){};
  ValuePtr(const ValuePtr &other) : ptr(other.ptr ? new T(*other.ptr) : nullptr) {}

  ValuePtr(std::unique_ptr<T> &&other) : ptr(std::move(other)) {}

  ValuePtr &operator=(ValuePtr &&) = default;
  ValuePtr &operator=(ValuePtr const &other) {
    *this = ValuePtr(other);
    return *this;
  }
  ValuePtr &operator=(std::nullptr_t) {
    ptr = nullptr;
    return *this;
  }

  T &operator*() const { return *ptr; }
  T *operator->() const { return ptr.get(); }
  T *get() const { return ptr.get(); }

private:
  std::unique_ptr<T> ptr;
};

template <typename T>
bool operator==(ValuePtr<T> const &lhs, std::nullptr_t) noexcept {
  return lhs.get() == nullptr;
}

template <typename T>
bool operator!=(std::nullptr_t, ValuePtr<T> const &rhs) noexcept {
  return !(rhs == nullptr);
}

} // namespace memory

} // namespace opzioni

#endif // OPZIONI_MEMORY_H
