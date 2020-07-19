#ifndef OPZIONI_TYPES_H
#define OPZIONI_TYPES_H

#include <any>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include <fmt/format.h>

#include "converters.hpp"

namespace opzioni {

// +-----------------------+
// | General utility types |
// +-----------------------+

template <typename T> class ValuePtr {
public:
  ValuePtr() = default;
  ValuePtr(ValuePtr &&) = default;
  ValuePtr(const ValuePtr &other) : ptr(other.ptr ? new T(*other.ptr) : nullptr) {}

  ValuePtr(std::unique_ptr<T> &&other) : ptr(std::move(other)) {}

  ValuePtr &operator=(ValuePtr &&) = default;
  ValuePtr &operator=(ValuePtr const &other) {
    *this = ValuePtr(other);
    return *this;
  }

  T &operator*() const { return *ptr; }
  T *operator->() const { return ptr.get(); }
  T *get() const { return ptr.get(); }

private:
  std::unique_ptr<T> ptr;
};

template <typename T>[[no_discard]] bool operator==(ValuePtr<T> const &lhs, std::nullptr_t) noexcept {
  return lhs.get() == nullptr;
}

template <typename T>[[no_discard]] bool operator!=(std::nullptr_t, ValuePtr<T> const &rhs) noexcept {
  return !(rhs == nullptr);
}

// +-------------------+
// | User-facing types |
// +-------------------+

struct Command {
  std::string name;
  std::string epilog;
  std::string description;
};

struct Arg {
  std::string name{};
  std::optional<std::string> terse = std::nullopt;
  std::string help{};
  bool required = false;
};

struct ArgValue {
  std::string value{};

  template <typename T> T as() const { return convert<T>(value); }
};

struct ArgMap {
  ArgValue operator[](std::string name) const {
    if (!args.contains(name)) throw UnknownArgument(fmt::format("Could not find argument `{}`", name));
    return args.at(name);
  }

  template <typename T> T get_if_else(T &&desired, std::string name, T&& otherwise) const {
    // mainly intended for flags which are not only true or false
    if (args.contains(name)) return desired;
    else return otherwise;
  }

  template <typename T> T value_or(std::string name, T&& otherwise) const {
    if (auto const arg = args.find(name); arg != args.end()) return arg->second.as<T>();
    else return otherwise;
  }

  template <typename T> T as(std::string name) const {
    auto const arg = (*this)[name];
    return arg.as<T>();
  }

  auto size() const noexcept { return this->args.size(); }

  std::string cmd_name;
  ValuePtr<ArgMap> subcmd;
  std::map<std::string, ArgValue> args;
};

// +------------------------+
// | Specific utility types |
// +------------------------+

struct ParseResult {
  std::string cmd_name;
  ValuePtr<ParseResult> subcmd;
  std::vector<std::string> positional;
  std::map<std::string, std::string> options;
  std::set<std::string> flags;
};

struct ParsedOption {
  size_t num_of_dashes;
  std::string name;
  std::optional<std::string> value;
};

} // namespace opzioni

#endif // OPZIONI_TYPES_H
