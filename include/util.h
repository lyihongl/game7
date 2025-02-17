#include <stdexcept>
#include <string>
#include <variant>

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
};