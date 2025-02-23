#pragma once
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

namespace yhl_util {
struct error
{
    std::string message;
};

template<typename T, typename E = error>
class yhl_result
{
  public:
    yhl_result()
      : data(std::in_place_index<1>, E{})
    {
    }
    yhl_result(const T& d)
      : data(std::in_place_index<0>, d)
    {
    }
    yhl_result(T&& d)
      : data(std::in_place_index<0>, d)
    {
    }
    template<typename U>
    yhl_result(U&& d)
    {
        yhl_result(std::forward<U>(d));
    }
    yhl_result(const E& e)
      : data(std::in_place_index<1>, e)
    {
    }
    yhl_result(E&& e)
      : data(std::in_place_index<1>, e)
    {
    }
    bool has_value() const { return data.index() == 0; }

    const T& value() const
    {
        if (!has_value()) {
            throw std::runtime_error("yhl_result does not contain value");
        }
        return std::get<0>(data);
    }
    const E& error() const
    {
        if (has_value()) {
            throw std::runtime_error("yhl_result does not contain error");
        }
        return std::get<1>(data);
    }
    explicit operator bool() const { return has_value(); }

  private:
    std::variant<T, E> data;
};
template<typename T>
class out
{
    T* ptr; // Pointer to the output variable

  public:
    // Constructor: Takes a reference to the output variable
    explicit out(T& output)
      : ptr(&output)
    {
    }

    // Disallow default construction (output must be provided)
    out() = delete;

    // Assignment operator: Assigns a value to the output variable
    out& operator=(const T& value)
    {
        if (!ptr) {
            throw std::runtime_error("out<T> is not initialized");
        }
        *ptr = value;
        return *this;
    }

    // Implicit conversion to T& for direct access
    operator T&()
    {
        if (!ptr) {
            throw std::runtime_error("out<T> is not initialized");
        }
        return *ptr;
    }

    // Disallow copying (to prevent misuse)
    out(const out&) = delete;
    out& operator=(const out&) = delete;

    // Allow moving (optional, for advanced use cases)
    out(out&& other) noexcept
      : ptr(other.ptr)
    {
        other.ptr = nullptr;
    }
    out& operator=(out&& other) noexcept
    {
        if (this != &other) {
            ptr = other.ptr;
            other.ptr = nullptr;
        }
        return *this;
    }
};

};