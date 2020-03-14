#pragma once

#include <any>
#include <tuple>
#include <string>
#include <variant>
#include <optional>
#include <exception>
#include <functional>
#include <type_traits>

namespace parsec {
template<typename T1, typename T2> struct Concat;
template<typename T1, typename T2> struct Addition;

template<typename T1, typename T2> using Concat_t = typename Concat<T1, T2>::type;
template<typename T1, typename T2> using Addition_t = typename Addition<T1, T2>::type;

template<typename T1, typename ...T2s> struct Concat<T1, std::tuple<T2s...>> {
    using type = std::tuple<T1, T2s...>;
};
template<typename ...T1s, typename ...T2s> struct Concat<std::tuple<T1s...>, std::tuple<T2s...>> {
    using type = std::tuple<T1s..., T2s...>;
};
template<typename T1, typename ...T2s> struct Concat<T1, std::variant<T2s...>> {
    using type = std::variant<T1, T2s...>;
};
template<typename ...T1s, typename ...T2s> struct Concat<std::variant<T1s...>, std::variant<T2s...>> {
    using type = std::variant<T1s..., T2s...>;
};

template<typename T1, typename T2> struct Addition {
    using type = std::conditional_t<std::is_same_v<T1, T2>, T1, std::variant<T1, T2>>;
};
template<typename T1, typename T2> struct Addition<std::variant<T1>, T2> {
    using type = std::conditional_t<std::is_same_v<T1, T2>, std::variant<T1>, std::variant<T1 ,T2>>;
};
template<typename T1, typename T2> struct Addition<T1, std::variant<T2>> {
    using type = std::conditional_t<std::is_same_v<T1, T2>, std::variant<T1>, std::variant<T1 ,T2>>;
};
template<typename T1, typename ...T1s, typename T2> struct Addition<std::variant<T1, T1s...>, T2> {
    using type = std::conditional_t<std::is_same_v<T1, T2>, std::variant<T1, T1s...>, Concat_t<T1, Addition_t<std::variant<T1s...>, T2>>>;
};
template<typename T1, typename T2, typename ...T2s> struct Addition<T1, std::variant<T2, T2s...>> {
    using type = std::conditional_t<std::is_same_v<T1, T2>, std::variant<T2, T2s...>, Concat_t<T2, Addition_t<std::variant<T2s>..., T1>>>;
};
template<typename T1, typename T2> struct Addition<std::variant<T1>, std::variant<T2>> {
    using type = std::conditional_t<std::is_same_v<T1, T2>, std::variant<T1>, std::variant<T1, T2>>;
};
template<typename T1, typename T2, typename ...T2s> struct Addition<std::variant<T1>, std::variant<T2, T2s...>> {
    using type = std::conditional_t<std::is_same_v<T1, T2>, std::variant<T2, T2s...>, Concat_t<T2, Addition_t<T1, std::variant<T2s...>>>>;
};
template<typename T1, typename ...T1s, typename ...T2s> struct Addition<std::variant<T1, T1s...>, std::variant<T2s...>> {
    using type = Concat_t<Addition_t<T1, std::variant<T2s...>>, Addition_t<std::variant<T1s...>, std::variant<T2s...>>>;
};

class ParsecComponentBase {
  public:
    virtual ~ParsecComponentBase() = default;
};

// Result must be a single type, Tuple or Variant
template<typename Result>
class ParsecComponent : public ParsecComponentBase {
  public:
    using ResultType = std::optional<Result>;

    virtual ~ParsecComponent() = default;

    ResultType operator()(const std::string &str, std::size_t &index) {
        return exec_(str, index);
    }

  private:
    std::function<ResultType(const std::string &str, std::size_t &index)> exec_;

    template<typename R, typename ...Args>
    ParsecComponent operator>>(std::function<R(Args...)> callback) {
        ParsecComponent<R> component;
        component->exec_ = [callback](const std::string &str, std::size_t &index) -> std::optional<R> {
            auto result = this->operator()(str, index);
            if (result) {
                return std::apply(callback, result.value());
            } else {
                return {};
            }
        };
        return component;
    }

    template<typename RhsResult>
    ParsecComponentBase operator+(ParsecComponent<RhsResult> &rhs) {
        using NewResult = Product_t<Result, RhsResult>;
        ParsecComponent<NewResult> component;
        component.exec_ = [this, rhs](const std::string &str, std::size_t &index) -> std::optional<NewResult> {
            auto lr = this->operator()(str, index);
            if (lr) {
                auto rr = rhs.operator()(str, index);
                if (rr) {
                    return std::tuple_cat(lr.value(), rr.value());
                } else {
                    return {};
                }
            } else {
                return {};
            }
        };
        return component;
    }

    template<typename RhsResult>
    ParsecComponentBase operator|(ParsecComponent<RhsResult> &rhs) {
        using NewResult = Addition_t<Result, RhsResult>;
        ParsecComponent<NewResult> component;
        component.exec_ = [this, rhs](const std::string &str, std::size_t &index) -> std::optional<NewResult> {
            auto lr = this->operator()(str, index);
            if (lr) {
                return lr;
            } else {
                return rhs.operator()(str, index);
            }
        };
        return component;
    }
};

template<typename Result>
class Parsec final {
  public:
    Parsec() = default;

    Result operator()(const std::string &str, std::size_t &index) {
        if (component_) {
            auto r = static_cast<ParsecComponent<Result>*>(component_)->operator()(str, index);
            if (r) {
                return r;
            } else {
                throw std::runtime_error("parse error at " + std::to_string(index));
            }
        }
    }

  private:
    ParsecComponentBase *component_;
};
}
